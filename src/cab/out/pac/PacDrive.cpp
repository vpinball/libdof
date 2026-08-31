#include "PacDrive.h"
#include "PacDriveSingleton.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../Cabinet.h"
#include "../Output.h"
#include "../../../cab/CabinetOwner.h"

#include <algorithm>
#include <cstring>
#include <thread>

namespace DOF
{

PacDrive::PacDriveUnit* PacDrive::s_pacDriveInstance = nullptr;

PacDrive::PacDrive() { SetName("PacDrive"); }

PacDrive::~PacDrive() { }

void PacDrive::Init(Cabinet* cabinet)
{
   AddOutputs();

   if (!s_pacDriveInstance)
   {
      s_pacDriveInstance = new PacDriveUnit();
   }

   s_pacDriveInstance->Init(cabinet);
   Log::Write("PacDrive initialized and updater thread started.");
}

void PacDrive::Finish()
{
   if (s_pacDriveInstance)
   {
      s_pacDriveInstance->Finish();
      s_pacDriveInstance->ShutdownLighting();
   }
   Log::Write("PacDrive finished and updater thread stopped.");
}

void PacDrive::Update()
{
   if (s_pacDriveInstance)
   {
      s_pacDriveInstance->TriggerPacDriveUpdaterThread();
   }
}

void PacDrive::AddOutputs()
{
   OutputList* outputs = GetOutputs();
   for (int i = 1; i <= 16; i++)
   {
      bool found = false;
      for (const auto& output : *outputs)
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
         outputs->push_back(newOutput);
      }
   }
}

void PacDrive::OnOutputValueChanged(IOutput* output)
{
   IOutput* on = output;

   if (!on || on->GetNumber() < 1 || on->GetNumber() > 16)
   {
      Log::Exception(
         StringExtensions::Build("PacDrive output numbers must be in the range of 1-16. The supplied output number {0} is out of range.", std::to_string(on ? on->GetNumber() : -1)));
      return;
   }

   if (s_pacDriveInstance)
   {
      s_pacDriveInstance->UpdateValue(on);
   }
}

tinyxml2::XMLElement* PacDrive::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
      element->SetAttribute("Name", GetName().c_str());

   return element;
}

bool PacDrive::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const char* name = element->Attribute("Name");
   if (name)
      SetName(name);

   return true;
}

PacDrive::PacDriveUnit::PacDriveUnit()
   : m_newValue(0)
   , m_currentValue(0)
   , m_updateRequired(true)
   , m_index(-1)
   , m_keepPacDriveUpdaterAlive(false)
   , m_triggerUpdate(false)
{
}

PacDrive::PacDriveUnit::~PacDriveUnit() { }

void PacDrive::PacDriveUnit::Init(Cabinet* cabinet)
{
   m_index = PacDriveSingleton::GetInstance().PacDriveGetIndex();
   if (m_index >= 0)
   {
      StartPacDriveUpdaterThread();
   }
}

void PacDrive::PacDriveUnit::Finish()
{
   TerminatePacDriveUpdaterThread();
   ShutdownLighting();
}

void PacDrive::PacDriveUnit::UpdateValue(IOutput* output)
{
   if (!output || output->GetNumber() < 1 || output->GetNumber() > 16)
      return;

   int zeroBasedOutputNumber = output->GetNumber() - 1;
   uint16_t mask = static_cast<uint16_t>(1 << zeroBasedOutputNumber);

   std::lock_guard<std::mutex> lock(m_valueChangeLocker);

   if (output->GetOutput() != 0)
   {
      m_newValue |= mask;
   }
   else
   {
      m_newValue &= static_cast<uint16_t>(~mask);
   }
   m_updateRequired = true;
}

void PacDrive::PacDriveUnit::TriggerPacDriveUpdaterThread()
{
   {
      std::lock_guard<std::mutex> lock(m_pacDriveUpdaterThreadLocker);
      m_triggerUpdate = true;
   }
   m_updateCondition.notify_one();
}

void PacDrive::PacDriveUnit::ShutdownLighting()
{
   if (m_index >= 0)
   {
      PacDriveSetLEDStates(0);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
}

void PacDrive::PacDriveUnit::StartPacDriveUpdaterThread()
{
   std::lock_guard<std::mutex> lock(m_pacDriveUpdaterThreadLocker);
   if (!IsUpdaterThreadAlive())
   {
      m_keepPacDriveUpdaterAlive = true;
      m_updaterThreadFinished = false;
      m_pacDriveUpdater = std::thread(&PacDriveUnit::PacDriveUpdaterDoIt, this);
   }
}

void PacDrive::PacDriveUnit::TerminatePacDriveUpdaterThread()
{
   {
      std::lock_guard<std::mutex> lock(m_pacDriveUpdaterThreadLocker);
      m_keepPacDriveUpdaterAlive = false;
      m_triggerUpdate = true;
   }
   m_updateCondition.notify_all();

   if (m_pacDriveUpdater.joinable())
   {
      auto start = std::chrono::steady_clock::now();
      while (!m_updaterThreadFinished && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      if (m_updaterThreadFinished)
         m_pacDriveUpdater.join();
      else
         m_pacDriveUpdater.detach();
   }
}

void PacDrive::PacDriveUnit::PacDriveUpdaterDoIt()
{
   int failCount = 0;
   while (m_keepPacDriveUpdaterAlive)
   {
      try
      {
         if (m_index >= 0)
         {
            SendPacDriveUpdate();
         }
         failCount = 0;
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("Error occurred when updating PacDrive: {0}", e.what()));
         failCount++;

         if (failCount > MAX_UPDATE_FAIL_COUNT)
         {
            Log::Exception(StringExtensions::Build("More than {0} consecutive updates failed for PacDrive. Updater thread will terminate.", std::to_string(MAX_UPDATE_FAIL_COUNT)));
            m_keepPacDriveUpdaterAlive = false;
         }
      }

      if (m_keepPacDriveUpdaterAlive)
      {
         std::unique_lock<std::mutex> lock(m_pacDriveUpdaterThreadLocker);
         m_updateCondition.wait_for(lock, std::chrono::milliseconds(50), [this] { return m_triggerUpdate.load() || !m_keepPacDriveUpdaterAlive; });
      }
      m_triggerUpdate = false;
   }
   m_updaterThreadFinished = true;
}

void PacDrive::PacDriveUnit::SendPacDriveUpdate()
{
   if (m_index < 0)
      return;

   std::lock_guard<std::mutex> updateLock(m_pacDriveUpdateLocker);
   std::lock_guard<std::mutex> valueLock(m_valueChangeLocker);

   if (!m_updateRequired)
      return;

   CopyNewToCurrent();
   m_updateRequired = false;

   PacDriveSetLEDStates(m_currentValue);
}

void PacDrive::PacDriveUnit::CopyNewToCurrent() { m_currentValue = m_newValue; }

bool PacDrive::PacDriveUnit::IsUpdaterThreadAlive() const { return m_pacDriveUpdater.joinable(); }

bool PacDrive::PacDriveUnit::PacDriveSetLEDStates(uint16_t data)
{
   if (m_index < 0)
      return false;

   return PacDriveSingleton::GetInstance().PacDriveUHIDSetLEDStates(m_index, data);
}

}