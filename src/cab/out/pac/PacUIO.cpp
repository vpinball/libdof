#include "PacUIO.h"
#include "PacDriveSingleton.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../Output.h"
#include "../../toys/ToyList.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <algorithm>
#include <chrono>

namespace DOF
{

std::map<int, PacUIO::PacUIOUnit*> PacUIO::s_pacUIOUnits;
bool PacUIO::s_unitsInitialized = false;

void PacUIO::InitializeUnits()
{
   if (!s_unitsInitialized)
   {
      for (int i = 0; i <= 2; i++)
      {
         s_pacUIOUnits[i] = new PacUIOUnit(i);
      }
      s_unitsInitialized = true;
   }
}

void PacUIO::SetId(int value)
{
   if (value < 0 || value > 1)
   {
      throw std::runtime_error(StringExtensions::Build("PacUIO Ids must be between 0-1. The supplied Id {0} is out of range.", std::to_string(value)));
   }

   std::lock_guard<std::mutex> lock(m_idUpdateLocker);
   if (m_id != value)
   {
      if (GetName().empty() || GetName() == StringExtensions::Build("PacUIO {0:0}", std::to_string(m_id)))
      {
         SetName(StringExtensions::Build("PacUIO {0:0}", std::to_string(value)));
      }
      m_id = value;
   }
}

void PacUIO::Update()
{
   InitializeUnits();
   s_pacUIOUnits.at(m_id)->TriggerPacUIOUpdaterThread();
}

void PacUIO::Init(Cabinet* cabinet)
{
   AddOutputs();
   InitializeUnits();
   s_pacUIOUnits.at(m_id)->Init(cabinet);
   Log::Write(StringExtensions::Build("PacUIO Id:{0} initialized and updater thread started.", std::to_string(m_id)));
}

void PacUIO::Finish()
{
   InitializeUnits();
   s_pacUIOUnits.at(m_id)->Finish();
   s_pacUIOUnits.at(m_id)->ShutdownLighting();
   Log::Write(StringExtensions::Build("PacUIO Id:{0} finished and updater thread stopped.", std::to_string(m_id)));
}

void PacUIO::AddOutputs()
{
   OutputList* outputs = GetOutputs();
   for (int i = 1; i <= 96; i++)
   {
      bool found = false;
      for (IOutput* output : *outputs)
      {
         if (output->GetNumber() == i)
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         Output* newOutput = new Output();
         newOutput->SetName(StringExtensions::Build("{0}.{1:00}", GetName(), std::to_string(i)));
         newOutput->SetNumber(i);
         outputs->Add(newOutput);
      }
   }
}

void PacUIO::OnOutputValueChanged(IOutput* output)
{
   IOutput* on = output;

   if (!on || on->GetNumber() < 1 || on->GetNumber() > 96)
   {
      Log::Exception(
         StringExtensions::Build("PacUIO output numbers must be in the range of 1-96. The supplied output number {0} is out of range.", std::to_string(on ? on->GetNumber() : -1)));
      return;
   }

   InitializeUnits();
   PacUIOUnit* s = s_pacUIOUnits.at(m_id);
   s->UpdateValue(on);
}

PacUIO::PacUIO() { }

PacUIO::PacUIO(int id) { SetId(id); }

PacUIO::~PacUIO() { }

tinyxml2::XMLElement* PacUIO::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
      element->SetAttribute("Name", GetName().c_str());

   element->SetAttribute("Id", m_id);

   return element;
}

bool PacUIO::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const char* name = element->Attribute("Name");
   if (name)
      SetName(name);

   int id = element->IntAttribute("Id", -1);
   if (id != -1)
      SetId(id);

   return true;
}

PacUIO::PacUIOUnit::PacUIOUnit(int id)
   : m_id(id)
{
   m_pdSingleton = &PacDriveSingleton::GetInstance();
   m_index = PacDriveSingleton::GetInstance().PacUIOGetIndexForDeviceId(id);

   std::fill_n(m_newValue, 96, 0);
   std::fill_n(m_currentValue, 96, 0);
   std::fill_n(m_lastValueSent, 96, 0);
   std::fill_n(m_lastStateSent, 96, false);

   InitUnit();
}

PacUIO::PacUIOUnit::~PacUIOUnit()
{
   if (m_keepPacUIOUpdaterAlive.load())
   {
      Finish();
   }
}

void PacUIO::PacUIOUnit::Init(Cabinet* cabinet) { StartPacUIOUpdaterThread(); }

void PacUIO::PacUIOUnit::Finish()
{
   TerminatePacUIOUpdaterThread();
   ShutdownLighting();
}

void PacUIO::PacUIOUnit::UpdateValue(IOutput* output)
{
   int outputNumber = output->GetNumber();
   if (outputNumber < 1 || outputNumber > 96)
      return;

   int zeroBasedOutputNumber = outputNumber - 1;
   std::lock_guard<std::mutex> lock(m_valueChangeLocker);

   if (m_newValue[zeroBasedOutputNumber] != output->GetOutput())
   {
      m_newValue[zeroBasedOutputNumber] = output->GetOutput();
      m_updateRequired = true;
   }
}

void PacUIO::PacUIOUnit::CopyNewToCurrent()
{
   std::lock_guard<std::mutex> lock(m_valueChangeLocker);
   std::copy(m_newValue, m_newValue + 96, m_currentValue);
}

bool PacUIO::PacUIOUnit::IsUpdaterThreadAlive() { return m_pacUIOUpdater.joinable(); }

void PacUIO::PacUIOUnit::StartPacUIOUpdaterThread()
{
   std::lock_guard<std::mutex> lock(m_pacUIOUpdaterThreadLocker);
   if (!IsUpdaterThreadAlive())
   {
      m_keepPacUIOUpdaterAlive.store(true);
      m_updaterThreadFinished = false;
      m_pacUIOUpdater = std::thread(&PacUIOUnit::PacUIOUpdaterDoIt, this);
   }
}

void PacUIO::PacUIOUnit::TerminatePacUIOUpdaterThread()
{
   std::unique_lock<std::mutex> lock(m_pacUIOUpdaterThreadLocker);
   if (m_pacUIOUpdater.joinable())
   {
      m_keepPacUIOUpdaterAlive.store(false);
      TriggerPacUIOUpdaterThread();
      lock.unlock();

      auto start = std::chrono::steady_clock::now();
      while (!m_updaterThreadFinished && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      if (m_updaterThreadFinished)
         m_pacUIOUpdater.join();
      else
         m_pacUIOUpdater.detach();
   }
}

void PacUIO::PacUIOUnit::TriggerPacUIOUpdaterThread()
{
   m_triggerUpdate.store(true);
   m_triggerCondition.notify_one();
}

void PacUIO::PacUIOUnit::PacUIOUpdaterDoIt()
{
   try
   {
      ResetFadeTime();
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("A exception occurred while setting the fadetime for PacUIO {0} to 0.", std::to_string(m_index)));
      m_updaterThreadFinished = true;
      return;
   }

   int failCnt = 0;
   while (m_keepPacUIOUpdaterAlive.load())
   {
      try
      {
         if (IsPresent())
         {
            SendPacUIOUpdate();
         }
         failCnt = 0;
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("A error occurred when updating PacUIO {0}", std::to_string(m_id)));
         failCnt++;

         if (failCnt > MAX_UPDATE_FAIL_COUNT)
         {
            Log::Exception(StringExtensions::Build(
               "More than {0} consecutive updates failed for PacUIO {1}. Updater thread will terminate.", std::to_string(MAX_UPDATE_FAIL_COUNT), std::to_string(m_id)));
            m_keepPacUIOUpdaterAlive.store(false);
         }
      }

      if (m_keepPacUIOUpdaterAlive.load())
      {
         std::unique_lock<std::mutex> lock(m_pacUIOUpdaterThreadLocker);
         m_triggerCondition.wait_for(lock, std::chrono::milliseconds(50), [this]() { return m_triggerUpdate.load() || !m_keepPacUIOUpdaterAlive.load(); });
      }
      m_triggerUpdate.store(false);
   }
   m_updaterThreadFinished = true;
}

void PacUIO::PacUIOUnit::SendPacUIOUpdate()
{
   if (!IsPresent())
   {
      m_forceFullUpdate = true;
      return;
   }

   std::lock_guard<std::mutex> updateLock(m_pacUIOUpdateLocker);

   {
      std::lock_guard<std::mutex> valueLock(m_valueChangeLocker);
      if (!m_updateRequired && !m_forceFullUpdate)
         return;

      CopyNewToCurrent();
      m_updateRequired = false;
   }

   uint8_t intensityUpdatesRequired = 0;
   uint8_t stateUpdatesRequired = 0;

   if (!m_forceFullUpdate)
   {
      for (int g = 0; g < 12; g++)
      {
         bool stateUpdateRequired = false;
         for (int p = 0; p < 8; p++)
         {
            int o = g << 3 | p;
            if (m_currentValue[o] > 0)
            {
               if (m_currentValue[o] != m_lastValueSent[o])
               {
                  intensityUpdatesRequired++;
               }
               else if (!m_lastStateSent[o])
               {
                  stateUpdateRequired = true;
               }
            }
            else if (m_lastStateSent[o])
            {
               stateUpdateRequired = true;
            }
         }
         if (stateUpdateRequired)
            stateUpdatesRequired++;
      }
   }

   if (m_forceFullUpdate || (intensityUpdatesRequired + stateUpdatesRequired) > 50)
   {
      m_pdSingleton->PacLed64SetLEDIntensities(m_index, m_currentValue);
      std::copy(m_currentValue, m_currentValue + 96, m_lastValueSent);
      for (int i = 0; i < 96; i++)
      {
         m_lastStateSent[i] = (m_lastValueSent[i] > 0);
      }
   }
   else
   {
      for (int g = 0; g < 12; g++)
      {
         int mask = 0;
         bool stateUpdateRequired = false;
         for (int p = 0; p < 8; p++)
         {
            int o = g << 3 | p;
            if (m_currentValue[o] > 0)
            {
               if (m_currentValue[o] != m_lastValueSent[o])
               {
                  m_pdSingleton->PacLed64SetLEDIntensity(m_index, o, m_currentValue[o]);
                  m_lastStateSent[o] = true;
                  m_lastValueSent[o] = m_currentValue[o];
               }
               else if (!m_lastStateSent[o])
               {
                  mask |= (1 << p);
                  stateUpdateRequired = true;
                  m_lastStateSent[o] = true;
               }
            }
            else if (m_lastStateSent[o])
            {
               stateUpdateRequired = true;
               m_lastStateSent[o] = false;
               m_lastValueSent[o] = 0;
            }
         }
         if (stateUpdateRequired)
         {
            m_pdSingleton->PacLed64SetLEDStates(m_index, g + 1, static_cast<uint8_t>(mask));
         }
      }
   }
}

void PacUIO::PacUIOUnit::ShutdownLighting()
{
   Log::Write("PacUIO.ShutdownLighting");
   m_pdSingleton->PacLed64SetLEDStates(0, 0, 0);
   std::fill_n(m_lastStateSent, 96, false);
}

void PacUIO::PacUIOUnit::ResetFadeTime()
{
   Log::Write("PacUIO.ResetFadeTime");
   m_pdSingleton->PacLed64SetLEDFadeTime(m_index, 0);
}

bool PacUIO::PacUIOUnit::IsPresent()
{
   if (m_id < 0 || m_id > 3)
      return false;
   return m_index >= 0;
}

void PacUIO::PacUIOUnit::OnPacAttached(int index)
{
   m_index = m_pdSingleton->PacLed64GetIndexForDeviceId(m_id);
   InitUnit();
   TriggerPacUIOUpdaterThread();
}

void PacUIO::PacUIOUnit::OnPacRemoved(int index) { m_index = m_pdSingleton->PacLed64GetIndexForDeviceId(m_id); }

void PacUIO::PacUIOUnit::InitUnit()
{
   if (m_index >= 0)
   {
      std::fill_n(m_lastValueSent, 96, 0);
      m_pdSingleton->PacLed64SetLEDIntensities(m_index, m_lastValueSent);
      std::fill_n(m_lastStateSent, 96, false);
      std::fill_n(m_lastValueSent, 96, 0);
   }
}

}