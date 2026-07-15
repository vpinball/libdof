#include "LedWiz.h"
#include "LedWizOutput.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../Cabinet.h"
#include "../Output.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <regex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace DOF
{

std::vector<LedWiz::LWDEVICE> LedWiz::s_deviceList = { };
int LedWiz::s_startedUp = 0;
std::mutex LedWiz::s_startupLocker;

LedWiz::LedWiz()
{
   m_fp = nullptr;
   m_number = -1;
   m_minCommandIntervalMs = 10;
   m_minCommandIntervalMsSet = false;
}

LedWiz::LedWiz(int number)
   : LedWiz()
{
   SetNumber(number);
}

LedWiz::~LedWiz()
{
   if (m_fp)
   {
      AllOff();
   }
   DisconnectFromController();
}


void LedWiz::SetNumber(int value)
{
   if (value < 1 || value > 16)
   {
      Log::Write(StringExtensions::Build("LedWiz Numbers must be between 1-16. The supplied number {0} is out of range.", std::to_string(value)));
      return;
   }

   if (m_number != value)
   {
      if (GetName().empty() || GetName() == StringExtensions::Build("LedWiz {0:00}", std::to_string(m_number)))
      {
         SetName(StringExtensions::Build("LedWiz {0:00}", std::to_string(value)));
      }

      m_number = value;
   }
}

void LedWiz::SetMinCommandIntervalMs(int value)
{
   m_minCommandIntervalMs = std::clamp(value, 0, 1000);
   m_minCommandIntervalMsSet = true;
}

void LedWiz::Init(Cabinet* cabinet)
{
   Log::Write(StringExtensions::Build("Initializing LedWiz Nr. {0:00}", std::to_string(m_number)));
   AddOutputs();
   if (!m_minCommandIntervalMsSet && cabinet && cabinet->GetOwner() && cabinet->GetOwner()->HasConfigurationSetting("LedWizDefaultMinCommandIntervalMs"))
   {
      std::string value = cabinet->GetOwner()->GetConfigurationSetting("LedWizDefaultMinCommandIntervalMs");
      try
      {
         int intervalMs = std::stoi(value);
         SetMinCommandIntervalMs(intervalMs);
      }
      catch (const std::exception&)
      {
      }
   }

   ConnectToController();
   Log::Write(StringExtensions::Build("LedWiz Nr. {0:00} initialized and updater thread initialized.", std::to_string(m_number)));
}

void LedWiz::Finish()
{
   Log::Write(StringExtensions::Build("Finishing LedWiz Nr. {0:00}", std::to_string(m_number)));
   if (m_fp)
   {
      AllOff();
   }
   DisconnectFromController();
   Log::Write(StringExtensions::Build("LedWiz Nr. {0:00} finished and updater thread stopped.", std::to_string(m_number)));
}

void LedWiz::Update() { }

void LedWiz::OnOutputValueChanged(IOutput* output)
{
   if (!output || !m_fp)
      return;

   LedWizOutput* ledWizOutput = dynamic_cast<LedWizOutput*>(output);
   if (!ledWizOutput)
   {
      Log::Exception(StringExtensions::Build("The OutputValueChanged event handler for LedWiz {0:00} has been called by a sender which is not a LedWizOutput.", std::to_string(m_number)));
      return;
   }

   int ledWizOutputNumber = ledWizOutput->GetLedWizOutputNumber();
   if (ledWizOutputNumber < 1 || ledWizOutputNumber > 32)
   {
      Log::Exception(StringExtensions::Build("LedWiz output numbers must be in the range of 1-32. The supplied output number {0} is out of range.", std::to_string(ledWizOutputNumber)));
      return;
   }

   OutputList* outputs = GetOutputs();
   if (!outputs)
      return;

   std::vector<uint8_t> outputValues(32, 0);
   for (auto* out : *outputs)
   {
      LedWizOutput* lwOut = dynamic_cast<LedWizOutput*>(out);
      if (lwOut && lwOut->GetLedWizOutputNumber() >= 1 && lwOut->GetLedWizOutputNumber() <= 32)
      {
         outputValues[lwOut->GetLedWizOutputNumber() - 1] = lwOut->GetOutput();
      }
   }

   UpdateOutputs(outputValues);
}

void LedWiz::AddOutputs()
{
   OutputList* outputs = GetOutputs();
   if (!outputs)
      return;

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

bool LedWiz::IsConnected() const { return m_fp != nullptr; }

void LedWiz::AllOff()
{
   if (m_fp)
   {
      std::vector<uint8_t> buf = { 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00 };
      WriteUSB(buf);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
}

bool LedWiz::VerifySettings()
{
   if (m_number < 1 || m_number > 16)
   {
      Log::Write(StringExtensions::Build("LedWiz {0} cannot be configured. Valid LedWiz unit numbers are 1-16.", GetName()));
      return false;
   }
   return true;
}

void LedWiz::ConnectToController()
{
   if (!VerifySettings())
      return;

   for (const auto& device : s_deviceList)
   {
      if (device.unitNo == m_number)
      {
         m_fp = hid_open_path(device.path.c_str());
         if (m_fp)
         {
            m_path = device.path;
            Log::Write(StringExtensions::Build("LedWiz {0} connected successfully", GetName()));
            m_oldOutputValues.resize(32, 0);

            std::vector<uint8_t> initSba = { 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00 };
            WriteUSB(initSba);

            for (int ofs = 0; ofs < 32; ofs += 8)
            {
               std::vector<uint8_t> initPba = { 0x00, 49, 49, 49, 49, 49, 49, 49, 49 };
               WriteUSB(initPba);
            }

            return;
         }
      }
   }

   Log::Write(StringExtensions::Build("LedWiz {0} could not connect to device", GetName()));
}

void LedWiz::DisconnectFromController()
{
   if (m_fp)
   {
      hid_close(m_fp);
      m_fp = nullptr;
   }
   m_path.clear();
   Log::Write(StringExtensions::Build("LedWiz {0} disconnected", GetName()));
}

void LedWiz::UpdateOutputs(const std::vector<uint8_t>& newOutputValues)
{
   if (!m_fp)
      return;

   if (newOutputValues.size() != m_oldOutputValues.size())
   {
      m_oldOutputValues.resize(newOutputValues.size(), 0);
   }

   bool hasChanges = false;
   for (size_t i = 0; i < std::min(newOutputValues.size(), size_t(32)); ++i)
   {
      if (newOutputValues[i] != m_oldOutputValues[i])
      {
         hasChanges = true;
         break;
      }
   }

   if (hasChanges)
   {
      std::vector<uint8_t> sbaCmd = { 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00 };

      for (int i = 0; i < 32 && i < static_cast<int>(newOutputValues.size()); ++i)
      {
         int byteIndex = 2 + (i / 8);
         int bitIndex = i % 8;

         if (newOutputValues[i] > 0)
         {
            sbaCmd[byteIndex] |= (1 << bitIndex);
         }
      }

      WriteUSB(sbaCmd);

      for (int ofs = 0; ofs < 32; ofs += 8)
      {
         std::vector<uint8_t> pbaCmd = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

         for (int i = 0; i < 8; ++i)
         {
            int outputIndex = ofs + i;
            if (outputIndex < static_cast<int>(newOutputValues.size()))
            {
               pbaCmd[1 + i] = static_cast<uint8_t>((newOutputValues[outputIndex] * 49) / 255);
            }
         }

         WriteUSB(pbaCmd);
      }

      m_oldOutputValues = newOutputValues;
   }
}

bool LedWiz::WriteUSB(const std::vector<uint8_t>& data)
{
   if (!m_fp || data.empty())
      return false;

   int result = hid_write(m_fp, data.data(), data.size());
   if (result < 0)
      return false;

   return true;
}

void LedWiz::StartupLedWiz()
{
   std::lock_guard<std::mutex> lock(s_startupLocker);
   if (s_startedUp == 0)
   {
      s_deviceList.clear();

      hid_device_info* devs = hid_enumerate(0x0000, 0x0000);
      hid_device_info* curDev = devs;

      while (curDev)
      {
         std::string productName = GetDeviceProductName(curDev);
         std::string manufacturerName = GetDeviceManufacturerName(curDev);

         bool ok = false;

         if (curDev->vendor_id == 0xFAFA)
         {
            ok = true;
         }
         else if (curDev->vendor_id == 0x20A0)
         {
            std::regex zebsPattern("zebsboards", std::regex::ECMAScript | std::regex::icase);
            if (std::regex_search(manufacturerName, zebsPattern))
            {
               ok = true;
            }
         }

         if (ok)
         {
            int unitNo = (curDev->product_id & 0x0F) + 1;
            if (unitNo < 1 || unitNo > 16)
               unitNo = 1;

            s_deviceList.emplace_back(unitNo, curDev->path, productName);

            char pidHex[8];
            snprintf(pidHex, sizeof(pidHex), "%04X", curDev->product_id);
            Log::Write(StringExtensions::Build("Found LedWiz device: {0} (unit {1}) PID:{2} Path:{3}", productName, std::to_string(unitNo), std::string(pidHex), std::string(curDev->path)));
         }

         curDev = curDev->next;
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
         s_deviceList.clear();
      }
   }
}

void LedWiz::FindDevices() { StartupLedWiz(); }

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

std::vector<int> LedWiz::GetLedwizNumbers()
{
   StartupLedWiz();

   std::vector<int> numbers;
   for (const auto& device : s_deviceList)
   {
      numbers.push_back(device.unitNo);
   }
   return numbers;
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
   {
      SetName(nameElement->GetText());
   }

   const tinyxml2::XMLElement* numberElement = element->FirstChildElement("Number");
   if (numberElement && numberElement->GetText())
   {
      try
      {
         int number = std::stoi(numberElement->GetText());
         SetNumber(number);
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
         int interval = std::stoi(intervalElement->GetText());
         SetMinCommandIntervalMs(interval);
      }
      catch (...)
      {
      }
   }

   return true;
}

}