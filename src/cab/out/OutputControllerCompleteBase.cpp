#include "OutputControllerCompleteBase.h"
#include "Output.h"
#include "../../Log.h"
#include "../../general/StringExtensions.h"
#include <algorithm>
#include <cstring>
#include <condition_variable>
#include <memory>

namespace DOF
{

OutputControllerCompleteBase::OutputControllerCompleteBase()
   : m_updateRequired(false)
   , m_inUseState(InUseState::Startup)
   , m_keepUpdaterThreadAlive(false)
{
}

OutputControllerCompleteBase::~OutputControllerCompleteBase() { FinishUpdaterThread(); }

void OutputControllerCompleteBase::Init(Cabinet* cabinet)
{
   bool settingsValid = false;
   try
   {
      settingsValid = VerifySettings();
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception occurred when verifying settings for {0} {1}: {2}. Cannot initialize.", GetXmlElementName(), GetName(), e.what()));
      throw;
   }

   if (settingsValid)
   {
      SetupOutputs();
      InitUpdaterThread();
      Log::Write(StringExtensions::Build("{0} {1} initialized and updater thread started.", GetXmlElementName(), GetName()));
   }
   else
   {
      Log::Warning(StringExtensions::Build("Settings for {0} {1} are not correct. Cannot initialize.", GetXmlElementName(), GetName()));
   }
}

void OutputControllerCompleteBase::Finish()
{
   FinishUpdaterThread();
   Log::Write(StringExtensions::Build("{0} {1} finished and updater thread stopped.", GetXmlElementName(), GetName()));
}

void OutputControllerCompleteBase::Update()
{
   if (m_updateRequired)
      UpdaterThreadSignal();
}

void OutputControllerCompleteBase::SetValues(int firstOutput, const uint8_t* values, int valueCount)
{
   if (firstOutput < 0 || values == nullptr || valueCount <= 0)
      return;

   std::lock_guard<std::mutex> lock(m_valueChangeMutex);

   int requiredSize = firstOutput + valueCount;
   if (static_cast<int>(m_outputValues.size()) < requiredSize)
      m_outputValues.resize(requiredSize, 0);

   int maxCopyLength = static_cast<int>(m_outputValues.size()) - firstOutput;
   int copyLength = std::min(valueCount, maxCopyLength);

   if (copyLength > 0)
   {
      std::memcpy(&m_outputValues[firstOutput], values, copyLength);

      if (m_inUseState == InUseState::Startup)
         m_inUseState = InUseState::ValueChanged;

      m_updateRequired = true;
   }
}

void OutputControllerCompleteBase::SetupOutputs()
{
   int numberOfOutputs = std::max(0, GetNumberOfConfiguredOutputs());

   if (!GetOutputs())
      SetOutputs(new OutputList());

   std::lock_guard<std::mutex> lock(m_valueChangeMutex);

   auto* outputList = GetOutputs();
   std::vector<IOutput*> outputsToRemove;

   for (size_t i = 0; i < outputList->size(); ++i)
   {
      IOutput* output = (*outputList)[i];
      if (output && output->GetNumber() > numberOfOutputs)
         outputsToRemove.push_back(output);
   }

   for (IOutput* output : outputsToRemove)
      outputList->Remove(output);

   for (int i = 1; i <= numberOfOutputs; ++i)
   {
      if (!outputList->FindByNumber(i))
      {
         Output* output = new Output();
         output->SetNumber(i);
         output->SetOutput(0);

         std::string outputName = GetName().empty() ? "" : GetName();
         outputName += StringExtensions::Build(".{0:00}", std::to_string(i));
         output->SetName(outputName);

         outputList->Add(output);
      }
   }

   if (static_cast<int>(m_outputValues.size()) != numberOfOutputs)
      m_outputValues.resize(numberOfOutputs, 0);
   for (int i = 1; i <= numberOfOutputs; ++i)
   {
      IOutput* output = outputList->FindByNumber(i);
      if (output && i <= static_cast<int>(m_outputValues.size()))
         m_outputValues[i - 1] = output->GetOutput();
   }
}

void OutputControllerCompleteBase::RenameOutputs()
{
   auto* outputList = GetOutputs();
   if (outputList)
   {
      for (size_t i = 0; i < outputList->size(); ++i)
      {
         IOutput* output = (*outputList)[i];
         if (output)
         {
            std::string outputName = GetName().empty() ? "" : GetName();
            outputName += "." + std::to_string(output->GetNumber());
            output->SetName(outputName);
         }
      }
   }
}

void OutputControllerCompleteBase::OnNameChanged() { RenameOutputs(); }

tinyxml2::XMLElement* OutputControllerCompleteBase::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
   {
      tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
      nameElement->SetText(GetName().c_str());
      element->InsertEndChild(nameElement);
   }

   return element;
}

bool OutputControllerCompleteBase::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const char* name = element->Attribute("Name");
   if (name)
   {
      SetName(name);
   }
   else
   {
      const tinyxml2::XMLElement* nameElement = element->FirstChildElement("Name");
      if (nameElement && nameElement->GetText())
      {
         SetName(nameElement->GetText());
      }
   }

   return true;
}

void OutputControllerCompleteBase::OnOutputValueChanged(IOutput* output)
{
   if (!output)
      return;

   int outputNumber = output->GetNumber();
   uint8_t outputValue = output->GetOutput();

   if (outputNumber > 0 && outputNumber <= static_cast<int>(m_outputValues.size()))
   {
      std::lock_guard<std::mutex> lock(m_valueChangeMutex);

      if (m_outputValues[outputNumber - 1] != outputValue)
      {
         if (m_inUseState == InUseState::Startup)
            m_inUseState = InUseState::ValueChanged;

         m_outputValues[outputNumber - 1] = outputValue;

         m_updateRequired = true;

         UpdaterThreadSignal();
      }
   }
}

void OutputControllerCompleteBase::InitUpdaterThread()
{
   if (!IsUpdaterThreadActive())
   {
      m_keepUpdaterThreadAlive = true;
      m_updaterThreadFinished = false;
      try
      {
         m_updaterThread = std::make_unique<std::thread>(&OutputControllerCompleteBase::UpdaterThreadDoIt, this);
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("{0} {1} updater thread could not start: {2}", GetXmlElementName(), GetName(), e.what()));
         throw;
      }
   }
}

void OutputControllerCompleteBase::FinishUpdaterThread()
{
   if (m_updaterThread && m_updaterThread->joinable())
   {
      m_keepUpdaterThreadAlive = false;
      UpdaterThreadSignal();

      auto start = std::chrono::steady_clock::now();
      while (!m_updaterThreadFinished && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      if (!m_updaterThreadFinished)
         Log::Warning("Updater thread did not quit within timeout. Waiting for it to finish.");
      m_updaterThread->join();
      m_updaterThread.reset();
   }
}

void OutputControllerCompleteBase::UpdaterThreadSignal()
{
   std::lock_guard<std::mutex> lock(m_conditionMutex);
   m_updateCondition.notify_one();
}

bool OutputControllerCompleteBase::IsUpdaterThreadActive() const { return m_updaterThread && m_updaterThread->joinable(); }

void OutputControllerCompleteBase::UpdaterThreadDoIt()
{
   Log::Write(StringExtensions::Build("Updater thread started for {0} {1}", GetXmlElementName(), GetName()));

   try
   {
      try
      {
         ConnectToController();
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("Could not connect to controller. Thread will quit: {0}", e.what()));
         try
         {
            DisconnectFromController();
         }
         catch (...)
         {
         }
         m_updaterThreadFinished = true;
         return;
      }

      Log::Write(StringExtensions::Build("Updater thread connected to {0} {1}", GetXmlElementName(), GetName()));

      while (m_keepUpdaterThreadAlive)
      {
         std::vector<uint8_t> valuesToSend;
         {
            std::lock_guard<std::mutex> lock(m_valueChangeMutex);
            valuesToSend = m_outputValues;
         }

         bool updateOK = true;
         try
         {
            if (m_inUseState == InUseState::ValueChanged)
            {
               std::vector<uint8_t> zeroValues(valuesToSend.size(), 0);
               UpdateOutputs(zeroValues);

               m_inUseState = InUseState::Running;
            }

            if (m_inUseState == InUseState::Running)
               UpdateOutputs(valuesToSend);
         }
         catch (const std::exception& e)
         {
            Log::Exception(StringExtensions::Build("Could not send update: {0}. Will try again.", e.what()));
            updateOK = false;
         }

         if (!updateOK)
         {
            Log::Warning("Trying to reconnect to controller...");
            try
            {
               DisconnectFromController();
            }
            catch (...)
            {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            try
            {
               ConnectToController();
            }
            catch (...)
            {
               break;
            }
            Log::Write("Reconnected to controller");
         }

         if (m_keepUpdaterThreadAlive)
         {
            std::unique_lock<std::mutex> lock(m_conditionMutex);
            m_updateCondition.wait_for(lock, std::chrono::milliseconds(50), [this] { return !m_keepUpdaterThreadAlive || m_updateRequired; });
         }
         m_updateRequired = false;
      }

      try
      {
         if (m_inUseState != InUseState::Startup)
         {
            std::vector<uint8_t> zeroValues(m_outputValues.size(), 0);
            UpdateOutputs(zeroValues);
         }
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("Exception occurred while trying to turn off all outputs: {0}", e.what()));
      }

      try
      {
         DisconnectFromController();
      }
      catch (...)
      {
      }
      Log::Write("Updater thread disconnected and will terminate");
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception occurred in updater thread: {0}", e.what()));
   }
   m_updaterThreadFinished = true;
}

}
