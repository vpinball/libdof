#include "LedWiz.h"
#include "LedWizOutput.h"
#include "../../../Log.h"
#include "../../../general/MathExtensions.h"
#include "../../../general/StringExtensions.h"
#include "../../Cabinet.h"
#include "../../schedules/ScheduledSettings.h"

#include <algorithm>
#include <cstdint>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace DOF
{

std::vector<LedWiz::LWDEVICE> LedWiz::s_deviceList;
int LedWiz::s_startedUp = 0;
std::mutex LedWiz::s_startupLocker;
std::map<int, LedWiz::LedWizUnit*> LedWiz::s_ledWizUnits;

const uint8_t LedWiz::LedWizUnit::s_byteToLedWizValue[256] = { 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
   8, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17,
   17, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 26, 26, 26, 26,
   26, 26, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 33, 33, 33, 33, 33, 34, 34, 34, 34, 34, 35, 35, 35,
   35, 35, 36, 36, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44,
   44, 44, 44, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 49 };
const int LedWiz::LedWizUnit::s_retryWait[10] = { 1, 1, 1, 2, 2, 2, 3, 3, 5, 5 };

void LedWiz::InitializeUnits()
{
   if (s_ledWizUnits.empty())
   {
      for (int i = 1; i <= 16; i++)
         s_ledWizUnits[i] = new LedWizUnit(i);
   }
}

LedWiz::LedWiz()
   : m_number(-1)
   , m_minCommandIntervalMs(10)
   , m_minCommandIntervalMsSet(false)
{
   InitializeUnits();
   Log::Write(StringExtensions::Build("Opening {0}-bit LedWiz driver...", sizeof(void*) == 8 ? "64" : "32"));
   StartupLedWiz();
}

LedWiz::LedWiz(int number)
   : LedWiz()
{
   SetNumber(number);
}

LedWiz::~LedWiz()
{
   Log::Debug(StringExtensions::Build("Disposing LedWiz instance {0:00}.", std::to_string(m_number)));
   TerminateLedWiz();
}

void LedWiz::SetNumber(int value)
{
   if (!MathExtensions::IsBetween(value, 1, 16))
      throw std::runtime_error(StringExtensions::Build("LedWiz Numbers must be between 1-16. The supplied number {0} is out of range.", std::to_string(value)));

   std::lock_guard<std::mutex> lock(m_numberUpdateLocker);
   if (m_number != value)
   {
      if (StringExtensions::IsNullOrWhiteSpace(GetName()) || GetName() == StringExtensions::Build("LedWiz {0:00}", std::to_string(m_number)))
         SetName(StringExtensions::Build("LedWiz {0:00}", std::to_string(value)));

      m_number = value;
   }
}

void LedWiz::SetMinCommandIntervalMs(int value)
{
   m_minCommandIntervalMs = MathExtensions::Limit(value, 0, 1000);
   m_minCommandIntervalMsSet = true;
}

void LedWiz::Update() { s_ledWizUnits.at(m_number)->TriggerLedWizUpdaterThread(); }

void LedWiz::Init(Cabinet* cabinet)
{
   Log::Write(StringExtensions::Build("Initializing LedWiz Nr. {0:00}", std::to_string(m_number)));
   AddOutputs();
   if (!m_minCommandIntervalMsSet && cabinet && cabinet->GetOwner() && cabinet->GetOwner()->HasConfigurationSetting("LedWizDefaultMinCommandIntervalMs"))
   {
      std::string value = cabinet->GetOwner()->GetConfigurationSetting("LedWizDefaultMinCommandIntervalMs");
      try
      {
         SetMinCommandIntervalMs(std::stoi(value));
      }
      catch (const std::exception&)
      {
      }
   }
   s_ledWizUnits.at(m_number)->Init(cabinet, m_minCommandIntervalMs);
   Log::Write(StringExtensions::Build("LedWiz Nr. {0:00} initialized and updater thread initialized.", std::to_string(m_number)));
}

void LedWiz::Finish()
{
   Log::Write(StringExtensions::Build("Finishing LedWiz Nr. {0:00}", std::to_string(m_number)));
   s_ledWizUnits.at(m_number)->Finish();
   s_ledWizUnits.at(m_number)->ShutdownLighting();
   Log::Write(StringExtensions::Build("LedWiz Nr. {0:00} finished and updater thread stopped.", std::to_string(m_number)));
}

void LedWiz::AddOutputs()
{
   OutputList* outputs = GetOutputs();
   for (int i = 1; i <= 32; i++)
   {
      bool found = false;
      for (auto* output : *outputs)
      {
         LedWizOutput* lwOut = dynamic_cast<LedWizOutput*>(output);
         if (lwOut && lwOut->GetLedWizOutputNumber() == i)
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         LedWizOutput* newOutput = new LedWizOutput(i);
         newOutput->SetName(StringExtensions::Build("{0}.{1:00}", GetName(), std::to_string(i)));
         outputs->Add(newOutput);
      }
   }
}

void LedWiz::OnOutputValueChanged(IOutput* output)
{
   LedWizOutput* lwo = dynamic_cast<LedWizOutput*>(output);
   if (!lwo)
   {
      Log::Exception(StringExtensions::Build("The OutputValueChanged event handler for LedWiz {0:00} has been called by a sender which is not a LedWizOutput.", std::to_string(m_number)));
      return;
   }

   if (!MathExtensions::IsBetween(lwo->GetLedWizOutputNumber(), 1, 32))
   {
      Log::Exception(
         StringExtensions::Build("LedWiz output numbers must be in the range of 1-32. The supplied output number {0} is out of range.", std::to_string(lwo->GetLedWizOutputNumber())));
      return;
   }

   LedWizUnit* s = s_ledWizUnits.at(m_number);

   IOutput* recalculatedOutput = ScheduledSettings::GetInstance().GetNewRecalculatedOutput(output, 1, m_number - 1);
   lwo->SetOutput(recalculatedOutput->GetOutput());
   if (recalculatedOutput != output)
      delete recalculatedOutput;
   s->UpdateValue(lwo);
}

void LedWiz::StartupLedWiz()
{
   std::lock_guard<std::mutex> lock(s_startupLocker);
   if (s_startedUp == 0)
   {
      s_deviceList.clear();

      hid_device_info* devs = hid_enumerate(0x0000, 0x0000);
      for (hid_device_info* curDev = devs; curDev; curDev = curDev->next)
      {
         if (std::any_of(s_deviceList.begin(), s_deviceList.end(), [curDev](const LWDEVICE& d) { return d.path == curDev->path; }))
            continue;

         bool ok = true;
         std::string okBecause;

         std::string name = GetDeviceProductName(curDev);
         std::string manuf = GetDeviceManufacturerName(curDev);

         Log::Instrumentation("LedWizDiscovery",
            StringExtensions::Build(
               "Scanning HID at VID/PID: {0:X4}/{1:X4}, product string: {2}, manufacturer: {3}", std::to_string(curDev->vendor_id), std::to_string(curDev->product_id), name, manuf));

         if (curDev->vendor_id == 0xFAFA)
            okBecause = "recognized by LedWiz vendor ID";
         else if (curDev->vendor_id == 0x20A0 && std::regex_search(manuf, std::regex("zebsboards", std::regex::icase)))
            okBecause = "recognized by ZebsBoards vendor ID and manufacturer string";
         else
            ok = false;

         ok &= (curDev->product_id >= 0x00F0 && curDev->product_id < 0x00FF);

         int unitNo = curDev->product_id - 0x00EF;

         if (ok)
         {
            hid_device* fp = hid_open_path(curDev->path);
            if (fp)
            {
               int outputReportByteLength = GetOutputReportByteLength(fp);
               if (outputReportByteLength >= 0)
                  ok &= (outputReportByteLength == 9);
               hid_close(fp);
            }
         }

         if (ok)
         {
            for (const auto& d : s_deviceList)
            {
               if (d.unitNo == unitNo)
               {
                  ok = false;
                  Log::Warning(StringExtensions::Build("LedWiz discovery: \"{0}\" (VID/PID {1:X4}/{2:X4}, LedWiz unit #{3}) passed LedWiz recognition tests, but its unit number conflicts "
                                                       "with the previously discovered device \"{4}\"; \"{0}\" will be ignored for this session",
                     { name, std::to_string(curDev->vendor_id), std::to_string(curDev->product_id), std::to_string(unitNo), d.productName }));
               }
            }
         }

         if (ok)
         {
            Log::Write(StringExtensions::Build("LedWiz discovery: recognition tests passed for \"{0}\" (VID/PID {1:X4}/{2:X4}, manufacturer {3}) as LW unit #{4}; {5}",
               { name, std::to_string(curDev->vendor_id), std::to_string(curDev->product_id), manuf, std::to_string(unitNo), okBecause }));
            s_deviceList.emplace_back(unitNo, curDev->path, name);
         }
      }

      hid_free_enumeration(devs);
   }
   s_startedUp++;
}

void LedWiz::TerminateLedWiz()
{
   std::lock_guard<std::mutex> lock(s_startupLocker);
   if (s_startedUp > 0)
   {
      s_startedUp--;
      if (s_startedUp == 0)
      {
         for (auto& [number, s] : s_ledWizUnits)
            s->ShutdownLighting();
      }
   }
}

std::vector<int> LedWiz::GetLedwizNumbers()
{
   InitializeUnits();
   StartupLedWiz();

   std::vector<int> lst;
   for (const auto& d : s_deviceList)
      lst.push_back(d.unitNo);
   return lst;
}

void LedWiz::ClearDevices()
{
   for (auto& [number, s] : s_ledWizUnits)
      delete s;
   s_ledWizUnits.clear();

   std::lock_guard<std::mutex> lock(s_startupLocker);
   s_deviceList.clear();
   s_startedUp = 0;
}

std::string LedWiz::GetDeviceProductName(hid_device_info* dev)
{
   if (dev->product_string)
   {
#ifdef _WIN32
      int size = WideCharToMultiByte(CP_UTF8, 0, dev->product_string, -1, nullptr, 0, nullptr, nullptr);
      if (size > 0)
      {
         std::string str(size - 1, 0);
         WideCharToMultiByte(CP_UTF8, 0, dev->product_string, -1, &str[0], size, nullptr, nullptr);
         return str;
      }
      return "<not available>";
#else
      std::wstring wstr(dev->product_string);
      std::string str(wstr.begin(), wstr.end());
      return str;
#endif
   }
   return "<not available>";
}

std::string LedWiz::GetDeviceManufacturerName(hid_device_info* dev)
{
   if (dev->manufacturer_string)
   {
#ifdef _WIN32
      int size = WideCharToMultiByte(CP_UTF8, 0, dev->manufacturer_string, -1, nullptr, 0, nullptr, nullptr);
      if (size > 0)
      {
         std::string str(size - 1, 0);
         WideCharToMultiByte(CP_UTF8, 0, dev->manufacturer_string, -1, &str[0], size, nullptr, nullptr);
         return str;
      }
      return "<not available>";
#else
      std::wstring wstr(dev->manufacturer_string);
      std::string str(wstr.begin(), wstr.end());
      return str;
#endif
   }
   return "<not available>";
}

int LedWiz::GetOutputReportByteLength(hid_device* dev)
{
   unsigned char desc[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
   int len = hid_get_report_descriptor(dev, desc, sizeof(desc));
   if (len < 0)
      return -1;

   std::map<int, int> reportBits;
   int reportSize = 0;
   int reportCount = 0;
   int reportId = 0;

   for (int i = 0; i < len;)
   {
      uint8_t prefix = desc[i];
      if (prefix == 0xFE)
      {
         if (i + 1 >= len)
            break;
         i += 3 + desc[i + 1];
         continue;
      }

      int size = prefix & 0x03;
      if (size == 3)
         size = 4;
      if (i + 1 + size > len)
         break;

      uint32_t value = 0;
      for (int b = 0; b < size; b++)
         value |= static_cast<uint32_t>(desc[i + 1 + b]) << (8 * b);

      int type = (prefix >> 2) & 0x03;
      int tag = (prefix >> 4) & 0x0F;

      if (type == 1)
      {
         if (tag == 7)
            reportSize = value;
         else if (tag == 8)
            reportId = value;
         else if (tag == 9)
            reportCount = value;
      }
      else if (type == 0 && tag == 9)
         reportBits[reportId] += reportSize * reportCount;

      i += 1 + size;
   }

   int maxBytes = 0;
   for (const auto& [id, bits] : reportBits)
      maxBytes = std::max(maxBytes, (bits + 7) / 8);

   return maxBytes + 1;
}

tinyxml2::XMLElement* LedWiz::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
   {
      tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
      nameElement->SetText(GetName().c_str());
      element->InsertEndChild(nameElement);
   }

   tinyxml2::XMLElement* numberElement = doc.NewElement("Number");
   numberElement->SetText(m_number);
   element->InsertEndChild(numberElement);

   tinyxml2::XMLElement* intervalElement = doc.NewElement("MinCommandIntervalMs");
   intervalElement->SetText(m_minCommandIntervalMs);
   element->InsertEndChild(intervalElement);

   return element;
}

bool LedWiz::FromXml(const tinyxml2::XMLElement* element)
{
   const tinyxml2::XMLElement* nameElement = element->FirstChildElement("Name");
   if (nameElement && nameElement->GetText())
      SetName(nameElement->GetText());

   const tinyxml2::XMLElement* numberElement = element->FirstChildElement("Number");
   if (numberElement && numberElement->GetText())
   {
      try
      {
         SetNumber(std::stoi(numberElement->GetText()));
      }
      catch (...)
      {
         return false;
      }
   }

   const tinyxml2::XMLElement* intervalElement = element->FirstChildElement("MinCommandIntervalMs");
   if (intervalElement && intervalElement->GetText())
   {
      try
      {
         SetMinCommandIntervalMs(std::stoi(intervalElement->GetText()));
      }
      catch (...)
      {
      }
   }

   return true;
}

LedWiz::LedWizUnit::LedWizUnit(int number)
   : m_number(number)
   , m_fp(nullptr)
   , m_currentOutputValues { }
   , m_newAfterValueSwitches { }
   , m_newBeforeValueSwitches { }
   , m_currentAfterValueSwitches { }
   , m_currentBeforeValueSwitches { }
   , m_newValueUpdateRequired(true)
   , m_newSwitchUpdateBeforeValueUpdateRequired(true)
   , m_newSwitchUpdateAfterValueUpdateRequired(true)
   , m_currentValueUpdateRequired(true)
   , m_currentSwitchUpdateBeforeValueUpdateRequired(true)
   , m_currentSwitchUpdateAfterValueUpdateRequired(true)
   , m_updateRequired(true)
   , m_keepLedWizUpdaterAlive(false)
   , m_triggerUpdate(false)
   , m_updaterThreadFinished(false)
   , m_inUseState(InUseStates::Startup)
   , m_lastUpdate(std::chrono::steady_clock::now())
   , m_minCommandIntervalMs(1)
   , m_pbaTime()
{
   m_newOutputValues.fill(49);

   StartupLedWiz();

   for (const auto& d : s_deviceList)
   {
      if (d.unitNo == m_number)
      {
         m_path = d.path;
         m_fp = hid_open_path(m_path.c_str());
         break;
      }
   }
}

LedWiz::LedWizUnit::~LedWizUnit()
{
   TerminateLedWizUpdaterThread();
   if (m_fp)
      hid_close(m_fp);
}

void LedWiz::LedWizUnit::Init(Cabinet* cabinet, int minCommandIntervalMs)
{
   m_minCommandIntervalMs = minCommandIntervalMs;
   StartLedWizUpdaterThread();
}

void LedWiz::LedWizUnit::Finish()
{
   TerminateLedWizUpdaterThread();
   ShutdownLighting();
}

void LedWiz::LedWizUnit::UpdateValue(LedWizOutput* ledWizOutput)
{
   uint8_t v = s_byteToLedWizValue[ledWizOutput->GetOutput()];
   bool s = (v != 0);
   int zeroBasedOutputNumber = ledWizOutput->GetLedWizOutputNumber() - 1;
   int byteNr = zeroBasedOutputNumber >> 3;
   int bitNr = zeroBasedOutputNumber & 7;
   uint8_t mask = static_cast<uint8_t>(1 << bitNr);

   std::lock_guard<std::recursive_mutex> lock(m_valueChangeLocker);

   if (m_inUseState == InUseStates::Startup)
      m_inUseState = InUseStates::ValueChanged;

   if (s != ((m_newAfterValueSwitches[byteNr] & mask) != 0))
   {
      if (s == false)
      {
         mask = static_cast<uint8_t>(~mask);
         m_newAfterValueSwitches[byteNr] &= mask;
         m_newBeforeValueSwitches[byteNr] &= mask;
         m_newSwitchUpdateBeforeValueUpdateRequired = true;
         m_updateRequired = true;
      }
      else
      {
         if (v != m_newOutputValues[zeroBasedOutputNumber])
         {
            m_newOutputValues[zeroBasedOutputNumber] = v;
            m_newAfterValueSwitches[byteNr] |= mask;
            m_newValueUpdateRequired = true;
            m_newSwitchUpdateAfterValueUpdateRequired = true;
            m_updateRequired = true;
         }
         else
         {
            m_newAfterValueSwitches[byteNr] |= mask;
            m_newBeforeValueSwitches[byteNr] |= mask;
            m_newSwitchUpdateBeforeValueUpdateRequired = true;
            m_updateRequired = true;
         }
      }
   }
   else
   {
      if (s == true && v != m_newOutputValues[zeroBasedOutputNumber])
      {
         m_newOutputValues[zeroBasedOutputNumber] = v;
         m_newValueUpdateRequired = true;
         m_updateRequired = true;
      }
   }
}

void LedWiz::LedWizUnit::CopyNewToCurrent()
{
   std::lock_guard<std::recursive_mutex> lock(m_valueChangeLocker);

   m_currentValueUpdateRequired = m_newValueUpdateRequired;
   m_currentSwitchUpdateBeforeValueUpdateRequired = m_newSwitchUpdateBeforeValueUpdateRequired;
   m_currentSwitchUpdateAfterValueUpdateRequired = m_newSwitchUpdateAfterValueUpdateRequired;

   if (m_newValueUpdateRequired)
   {
      m_currentOutputValues = m_newOutputValues;
      m_newValueUpdateRequired = false;
   }

   if (m_newSwitchUpdateAfterValueUpdateRequired || m_newSwitchUpdateBeforeValueUpdateRequired)
   {
      m_currentAfterValueSwitches = m_newAfterValueSwitches;
      m_currentBeforeValueSwitches = m_newBeforeValueSwitches;
      m_newBeforeValueSwitches = m_currentAfterValueSwitches;
      m_newSwitchUpdateAfterValueUpdateRequired = false;
      m_newSwitchUpdateBeforeValueUpdateRequired = false;
   }
}

bool LedWiz::LedWizUnit::IsUpdaterThreadAlive() const { return m_ledWizUpdater.joinable() && !m_updaterThreadFinished; }

void LedWiz::LedWizUnit::StartLedWizUpdaterThread()
{
   std::lock_guard<std::mutex> lock(m_ledWizUpdaterThreadLocker);
   if (!IsUpdaterThreadAlive())
   {
      if (m_ledWizUpdater.joinable())
         m_ledWizUpdater.join();

      m_keepLedWizUpdaterAlive = true;
      m_updaterThreadFinished = false;
      m_ledWizUpdater = std::thread(&LedWizUnit::LedWizUpdaterDoIt, this);
   }
}

void LedWiz::LedWizUnit::TerminateLedWizUpdaterThread()
{
   if (m_ledWizUpdater.joinable())
   {
      m_keepLedWizUpdaterAlive = false;
      TriggerLedWizUpdaterThread();

      auto start = std::chrono::steady_clock::now();
      while (!m_updaterThreadFinished && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1000))
         std::this_thread::sleep_for(std::chrono::milliseconds(10));

      m_ledWizUpdater.join();
   }
}

void LedWiz::LedWizUnit::TriggerLedWizUpdaterThread()
{
   m_triggerUpdate = true;
   std::lock_guard<std::mutex> lock(m_ledWizUpdaterThreadLocker);
   m_updateCondition.notify_one();
}

void LedWiz::LedWizUnit::LedWizUpdaterDoIt()
{
   Log::Write(StringExtensions::Build("Updater thread for LedWiz {0:00} started.", std::to_string(m_number)));

   int failCnt = 0;
   while (m_keepLedWizUpdaterAlive)
   {
      try
      {
         if (IsPresent())
         {
            if (m_inUseState == InUseStates::ValueChanged)
            {
               const uint8_t sba[4] = { 0, 0, 0, 0 };
               SBA(sba);
               std::array<uint8_t, 32> pba;
               pba.fill(49);
               PBA(pba.data());
               m_inUseState = InUseStates::Running;
            }

            if (m_inUseState == InUseStates::Running)
               SendLedWizUpdate();
         }
         failCnt = 0;
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("A error occurred when updating LedWiz Nr. {0}: {1}", std::to_string(m_number), e.what()));
         failCnt++;
         if (failCnt > MAX_UPDATE_FAIL_COUNT)
         {
            Log::Exception(StringExtensions::Build(
               "More than {0} consecutive updates failed for LedWiz Nr. {1}. Updater thread will terminate.", std::to_string(MAX_UPDATE_FAIL_COUNT), std::to_string(m_number)));
            m_keepLedWizUpdaterAlive = false;
         }
      }

      if (m_keepLedWizUpdaterAlive)
      {
         std::unique_lock<std::mutex> lock(m_ledWizUpdaterThreadLocker);
         while (!m_triggerUpdate && m_keepLedWizUpdaterAlive)
            m_updateCondition.wait_for(lock, std::chrono::milliseconds(50));
      }
      m_triggerUpdate = false;
   }

   Log::Write(StringExtensions::Build("Updater thread for LedWiz {0:00} terminated.", std::to_string(m_number)));
   m_updaterThreadFinished = true;
}

void LedWiz::LedWizUnit::UpdateDelay()
{
   int ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastUpdate).count());
   if (ms < m_minCommandIntervalMs)
      std::this_thread::sleep_for(std::chrono::milliseconds(MathExtensions::Limit(m_minCommandIntervalMs - ms, 0, m_minCommandIntervalMs)));
   m_lastUpdate = std::chrono::steady_clock::now();
}

void LedWiz::LedWizUnit::SendLedWizUpdate()
{
   if (MathExtensions::IsBetween(m_number, 1, 16))
   {
      std::lock_guard<std::mutex> updateLock(m_ledWizUpdateLocker);
      {
         std::lock_guard<std::recursive_mutex> valueLock(m_valueChangeLocker);
         if (!m_updateRequired)
            return;
         CopyNewToCurrent();
         m_updateRequired = false;
      }

      bool pbaTimeout = (std::chrono::steady_clock::now() - m_pbaTime > std::chrono::milliseconds(1000));
      if (m_currentValueUpdateRequired || pbaTimeout)
      {
         if (m_currentSwitchUpdateBeforeValueUpdateRequired || pbaTimeout)
            SBA(m_currentBeforeValueSwitches.data());
         PBA(m_currentOutputValues.data());
      }
      if (m_currentSwitchUpdateAfterValueUpdateRequired || (m_currentSwitchUpdateBeforeValueUpdateRequired && !m_currentValueUpdateRequired) || pbaTimeout)
         SBA(m_currentAfterValueSwitches.data());
   }
}

void LedWiz::LedWizUnit::ShutdownLighting()
{
   const uint8_t sba[4] = { 0, 0, 0, 0 };
   SBA(sba);
}

void LedWiz::LedWizUnit::SBA(const uint8_t* b, uint8_t globalFlashSpeed)
{
   const uint8_t buf[9] = { 0, 64, b[0], b[1], b[2], b[3], globalFlashSpeed, 0, 0 };
   WriteUSB(buf, sizeof(buf));
}

void LedWiz::LedWizUnit::PBA(const uint8_t* b)
{
   for (int ofs = 0; ofs < 32; ofs += 8)
   {
      const uint8_t buf[9] = { 0, b[ofs + 0], b[ofs + 1], b[ofs + 2], b[ofs + 3], b[ofs + 4], b[ofs + 5], b[ofs + 6], b[ofs + 7] };
      WriteUSB(buf, sizeof(buf));
   }
   m_pbaTime = std::chrono::steady_clock::now();
}

void LedWiz::LedWizUnit::WriteUSB(const uint8_t* buf, size_t length)
{
   if (m_fp)
   {
      UpdateDelay();
      for (int tries = 0; tries < static_cast<int>(std::size(s_retryWait)); ++tries)
      {
         if (hid_write(m_fp, buf, length) == static_cast<int>(length))
            return;
         std::this_thread::sleep_for(std::chrono::milliseconds(s_retryWait[tries]));
      }
      Log::Error(StringExtensions::Build("LedWiz {0} WriteUSB failed after {1} retries", std::to_string(m_number), std::to_string(std::size(s_retryWait))));
   }
   else
      Log::Error(StringExtensions::Build("LedWiz {0} WriteUSB has no file handle", std::to_string(m_number)));
}

bool LedWiz::LedWizUnit::IsPresent() const
{
   if (!MathExtensions::IsBetween(m_number, 1, 16))
      return false;
   return std::any_of(s_deviceList.begin(), s_deviceList.end(), [this](const LWDEVICE& d) { return d.unitNo == m_number; });
}

}
