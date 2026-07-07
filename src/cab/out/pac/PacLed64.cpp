#include "PacLed64.h"
#include "PacDriveSingleton.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../Cabinet.h"
#include "../Output.h"
#include "../../../cab/CabinetOwner.h"

#include <algorithm>
#include <cstring>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <memory>

namespace DOF
{

std::map<int, PacLed64::PacLed64Unit*> PacLed64::s_pacLed64Units;
bool PacLed64::s_unitsInitialized = false;

void PacLed64::InitializeUnits()
{
   if (!s_unitsInitialized)
   {
      for (int i = 1; i <= 4; i++)
      {
         s_pacLed64Units[i] = new PacLed64Unit(i);
      }
      s_unitsInitialized = true;
   }
}

PacLed64::PacLed64()
   : m_id(-1)
   , m_minUpdateIntervalMsSet(false)
   , m_minUpdateIntervalMs(10)
   , m_fullUpdateThreshold(30)
{
}

PacLed64::PacLed64(int id)
   : PacLed64()
{
   SetId(id);
}

PacLed64::~PacLed64() { Finish(); }

void PacLed64::SetId(int value)
{
   if (value < 1 || value > 4)
   {
      throw std::runtime_error(StringExtensions::Build("PacLed64 Ids must be between 1-4. The supplied Id {0} is out of range.", std::to_string(value)));
   }

   std::lock_guard<std::mutex> lock(m_idUpdateLocker);
   if (m_id != value)
   {
      if (GetName().empty() || GetName() == StringExtensions::Build("PacLed64 {0:0}", std::to_string(m_id)))
      {
         SetName(StringExtensions::Build("PacLed64 {0:0}", std::to_string(value)));
      }
      m_id = value;
   }
}

void PacLed64::SetMinUpdateIntervalMs(int value)
{
   m_minUpdateIntervalMs = std::clamp(value, 0, 1000);
   m_minUpdateIntervalMsSet = true;
}

void PacLed64::SetFullUpdateThreshold(int value) { m_fullUpdateThreshold = std::clamp(value, 0, 64); }

void PacLed64::Init(Cabinet* cabinet)
{
   AddOutputs();

   if (!m_minUpdateIntervalMsSet && cabinet && cabinet->GetOwner() && cabinet->GetOwner()->HasConfigurationSetting("PacLedDefaultMinCommandIntervalMs"))
   {
      std::string value = cabinet->GetOwner()->GetConfigurationSetting("PacLedDefaultMinCommandIntervalMs");
      try
      {
         int intervalMs = std::stoi(value);
         SetMinUpdateIntervalMs(intervalMs);
      }
      catch (const std::exception&)
      {
      }
   }

   InitializeUnits();
   s_pacLed64Units.at(m_id)->SetFullUpdateThreshold(m_fullUpdateThreshold);
   s_pacLed64Units.at(m_id)->SetMinUpdateInterval(std::chrono::milliseconds(m_minUpdateIntervalMs));
   s_pacLed64Units.at(m_id)->Init(cabinet);

   Log::Write(StringExtensions::Build("PacLed64 Id:{0} initialized and updater thread started.", std::to_string(m_id)));
}

void PacLed64::Finish()
{
   if (s_pacLed64Units.find(m_id) != s_pacLed64Units.end())
   {
      s_pacLed64Units[m_id]->Finish();
      s_pacLed64Units[m_id]->ShutdownLighting();
   }
   Log::Write(StringExtensions::Build("PacLed64 Id:{0} finished and updater thread stopped.", std::to_string(m_id)));
}

void PacLed64::Update()
{
   InitializeUnits();
   s_pacLed64Units.at(m_id)->TriggerPacLed64UpdaterThread();
}

void PacLed64::AddOutputs()
{
   OutputList* outputs = GetOutputs();
   for (int i = 1; i <= 64; i++)
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
         outputs->Add(newOutput);
      }
   }
}

void PacLed64::OnOutputValueChanged(IOutput* output)
{
   IOutput* on = output;


   if (!on || on->GetNumber() < 1 || on->GetNumber() > 64)
   {
      Log::Exception(
         StringExtensions::Build("PacLed64 output numbers must be in the range of 1-64. The supplied output number {0} is out of range.", std::to_string(on ? on->GetNumber() : -1)));
      return;
   }

   InitializeUnits();
   PacLed64Unit* s = s_pacLed64Units.at(m_id);
   s->UpdateValue(on);
}

tinyxml2::XMLElement* PacLed64::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
      element->SetAttribute("Name", GetName().c_str());

   element->SetAttribute("Id", m_id);

   if (m_minUpdateIntervalMsSet)
      element->SetAttribute("MinUpdateIntervalMs", m_minUpdateIntervalMs);

   element->SetAttribute("FullUpdateThreshold", m_fullUpdateThreshold);

   return element;
}

bool PacLed64::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const char* name = element->Attribute("Name");
   if (name)
      SetName(name);

   int id = element->IntAttribute("Id", -1);
   if (id != -1)
      SetId(id);

   int minInterval = element->IntAttribute("MinUpdateIntervalMs", -1);
   if (minInterval != -1)
      SetMinUpdateIntervalMs(minInterval);

   int threshold = element->IntAttribute("FullUpdateThreshold", 30);
   SetFullUpdateThreshold(threshold);

   return true;
}

// PacLed64Unit Implementation
PacLed64::PacLed64Unit::PacLed64Unit(int id)
   : m_id(id)
   , m_fullUpdateThreshold(30)
   , m_minUpdateInterval(std::chrono::milliseconds(10))
   , m_index(-1)
   , m_newValue(64, 0)
   , m_currentValue(64, 0)
   , m_lastValueSent(64, 0)
   , m_lastStateSent(64, false)
   , m_updateRequired(true)
   , m_keepPacLed64UpdaterAlive(false)
   , m_triggerUpdate(false)
   , m_forceFullUpdate(true)
{
   std::fill(m_lastValueSent.begin(), m_lastValueSent.end(), 255);
}

PacLed64::PacLed64Unit::~PacLed64Unit() { Finish(); }

void PacLed64::PacLed64Unit::Init(Cabinet* cabinet)
{
   m_index = PacDriveSingleton::GetInstance().PacLed64GetIndexForDeviceId(m_id);

   if (m_index >= 0)
   {
      InitUnit();
      StartPacLed64UpdaterThread();
      Log::Write(StringExtensions::Build("PacLed64 device {0} initialized at index {1}", std::to_string(m_id), std::to_string(m_index)));
   }
   else
   {
      Log::Warning(StringExtensions::Build("PacLed64 device {0} not found", std::to_string(m_id)));
   }
}

void PacLed64::PacLed64Unit::Finish()
{
   TerminatePacLed64UpdaterThread();
   ShutdownLighting();

   m_index = -1;
}

void PacLed64::PacLed64Unit::UpdateValue(IOutput* output)
{
   if (!output || output->GetNumber() < 1 || output->GetNumber() > 64)
      return;

   int zeroBasedOutputNumber = output->GetNumber() - 1;
   std::lock_guard<std::mutex> lock(m_valueChangeLocker);


   if (m_newValue[zeroBasedOutputNumber] != output->GetOutput())
   {
      m_newValue[zeroBasedOutputNumber] = output->GetOutput();
      m_updateRequired = true;
      TriggerPacLed64UpdaterThread();
   }
}

void PacLed64::PacLed64Unit::TriggerPacLed64UpdaterThread()
{
   {
      std::lock_guard<std::mutex> lock(m_pacLed64UpdaterThreadLocker);
      m_triggerUpdate = true;
   }
   m_updateCondition.notify_one();
}

bool PacLed64::PacLed64Unit::IsPresent() const { return m_id >= 1 && m_id <= 4 && m_index >= 0; }

void PacLed64::PacLed64Unit::ShutdownLighting()
{
   if (IsPresent())
   {
      EnforceMinUpdateInterval();
      PacDriveSingleton::GetInstance().PacLed64SetLEDStates(0, 0, 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::fill(m_lastStateSent.begin(), m_lastStateSent.end(), false);
   }
}

void PacLed64::PacLed64Unit::StartPacLed64UpdaterThread()
{
   std::lock_guard<std::mutex> lock(m_pacLed64UpdaterThreadLocker);
   if (!IsUpdaterThreadAlive())
   {
      m_keepPacLed64UpdaterAlive = true;
      m_updaterThreadFinished = false;
      m_pacLed64Updater = std::thread(&PacLed64Unit::PacLed64UpdaterDoIt, this);
   }
}

void PacLed64::PacLed64Unit::TerminatePacLed64UpdaterThread()
{
   {
      std::lock_guard<std::mutex> lock(m_pacLed64UpdaterThreadLocker);
      m_keepPacLed64UpdaterAlive = false;
      m_triggerUpdate = true;
   }
   m_updateCondition.notify_all();

   if (m_pacLed64Updater.joinable())
   {
      auto start = std::chrono::steady_clock::now();
      while (!m_updaterThreadFinished && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      if (m_updaterThreadFinished)
         m_pacLed64Updater.join();
      else
         m_pacLed64Updater.detach();
   }
}

void PacLed64::PacLed64Unit::PacLed64UpdaterDoIt()
{
   try
   {
      ResetFadeTime();
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception occurred while setting fade time for PacLed64 {0} to 0: {1}", std::to_string(m_index), e.what()));
      m_updaterThreadFinished = true;
      return;
   }

   int failCount = 0;
   while (m_keepPacLed64UpdaterAlive)
   {
      try
      {
         if (IsPresent())
         {
            SendPacLed64Update();
         }
         failCount = 0;
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("Error occurred when updating PacLed64 {0}: {1}", std::to_string(m_id), e.what()));
         failCount++;

         if (failCount > MAX_UPDATE_FAIL_COUNT)
         {
            Log::Exception(StringExtensions::Build(
               "More than {0} consecutive updates failed for PacLed64 {1}. Updater thread will terminate.", std::to_string(MAX_UPDATE_FAIL_COUNT), std::to_string(m_id)));
            m_keepPacLed64UpdaterAlive = false;
         }
      }

      if (m_keepPacLed64UpdaterAlive)
      {
         std::unique_lock<std::mutex> lock(m_pacLed64UpdaterThreadLocker);
         m_updateCondition.wait_for(lock, std::chrono::milliseconds(50), [this] { return m_triggerUpdate.load() || !m_keepPacLed64UpdaterAlive; });
      }
      m_triggerUpdate = false;
   }
   m_updaterThreadFinished = true;
}

void PacLed64::PacLed64Unit::SendPacLed64Update()
{
   if (!IsPresent())
      return;

   std::lock_guard<std::mutex> updateLock(m_pacLed64UpdateLocker);
   std::lock_guard<std::mutex> valueLock(m_valueChangeLocker);

   if (!m_updateRequired && !m_forceFullUpdate)
      return;

   CopyNewToCurrent();
   m_updateRequired = false;

   uint8_t intensityUpdatesRequired = 0;
   uint8_t stateUpdatesRequired = 0;

   if (!m_forceFullUpdate)
   {
      for (int g = 0; g < 8; g++)
      {
         bool stateUpdateRequired = false;
         for (int p = 0; p < 8; p++)
         {
            int o = (g << 3) | p;
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

   if (m_forceFullUpdate || (intensityUpdatesRequired + stateUpdatesRequired) > m_fullUpdateThreshold)
   {
      EnforceMinUpdateInterval();
      PacDriveSingleton::GetInstance().PacLed64SetLEDIntensities(m_index, m_currentValue.data());
      m_lastCommand = std::chrono::steady_clock::now();

      m_lastValueSent = m_currentValue;
      for (int i = 0; i < 64; i++)
      {
         m_lastStateSent[i] = (m_lastValueSent[i] > 0);
      }
      m_forceFullUpdate = false;
   }
   else
   {
      for (int g = 0; g < 8; g++)
      {
         int mask = 0;
         bool stateUpdateRequired = false;

         for (int p = 0; p < 8; p++)
         {
            int o = (g << 3) | p;
            if (m_currentValue[o] > 0)
            {
               if (m_currentValue[o] != m_lastValueSent[o])
               {
                  EnforceMinUpdateInterval();
                  PacDriveSingleton::GetInstance().PacLed64SetLEDIntensity(m_index, o, m_currentValue[o]);
                  m_lastCommand = std::chrono::steady_clock::now();

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
            EnforceMinUpdateInterval();
            PacDriveSingleton::GetInstance().PacLed64SetLEDStates(m_index, g + 1, static_cast<uint8_t>(mask));
            m_lastCommand = std::chrono::steady_clock::now();
         }
      }
   }
}

void PacLed64::PacLed64Unit::CopyNewToCurrent() { m_currentValue = m_newValue; }

void PacLed64::PacLed64Unit::ResetFadeTime()
{
   EnforceMinUpdateInterval();
   PacDriveSingleton::GetInstance().PacLed64SetLEDFadeTime(m_index, 0);
   m_lastCommand = std::chrono::steady_clock::now();
}

void PacLed64::PacLed64Unit::EnforceMinUpdateInterval()
{
   if (m_minUpdateInterval == std::chrono::milliseconds::zero())
      return;

   auto now = std::chrono::steady_clock::now();
   auto elapsed = now - m_lastCommand;

   if (elapsed < m_minUpdateInterval)
   {
      auto sleepTime = m_minUpdateInterval - elapsed;
      if (sleepTime > std::chrono::milliseconds::zero() && sleepTime <= std::chrono::milliseconds(1000))
      {
         std::this_thread::sleep_for(sleepTime);
      }
   }
   m_lastCommand = std::chrono::steady_clock::now();
}

void PacLed64::PacLed64Unit::InitUnit()
{
   if (m_index >= 0)
   {
      std::lock_guard<std::mutex> lock(m_valueChangeLocker);
      std::fill(m_newValue.begin(), m_newValue.end(), 0);
      m_updateRequired = true;
   }
}

bool PacLed64::PacLed64Unit::IsUpdaterThreadAlive() const { return m_pacLed64Updater.joinable(); }

}