#include "PinscapePico.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
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

#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../Cabinet.h"

namespace DOF
{

std::vector<PinscapePico::Device*> PinscapePico::s_devices = { };

void PinscapePico::Initialize()
{
   if (!s_devices.empty())
      return;
   FindDevices();
}

PinscapePico::PinscapePico()
{
   m_dev = nullptr;
   m_number = -1;
   m_minCommandIntervalMs = 1;
   m_minCommandIntervalMsSet = false;
   m_lastUpdate = std::chrono::steady_clock::now();
}

PinscapePico::PinscapePico(int number)
   : PinscapePico()
{
   SetNumber(number);
}

PinscapePico::~PinscapePico() { }

void PinscapePico::SetNumber(int value)
{
   if (value < 1 || value > 16)
   {
      Log::Write(StringExtensions::Build("PinscapePico Unit Numbers must be between 1-16. The supplied number {0} is out of range.", std::to_string(value)));
      return;
   }

   if (GetName().empty() || GetName() == StringExtensions::Build("PinscapePico {0:00}", std::to_string(m_number)))
   {
      SetName(StringExtensions::Build("PinscapePico {0:00}", std::to_string(value)));
   }

   m_number = value;
}

void PinscapePico::SetMinCommandIntervalMs(int value)
{
   m_minCommandIntervalMs = std::clamp(value, 0, 1000);
   m_minCommandIntervalMsSet = true;
}

void PinscapePico::Init(Cabinet* cabinet)
{
   if (!m_minCommandIntervalMsSet)
   {
      m_minCommandIntervalMs = 0;
   }

   OutputControllerFlexCompleteBase::Init(cabinet);
   ConnectToController();
}

void PinscapePico::Finish()
{
   if (m_dev)
   {
      AllOff();
   }
   DisconnectFromController();
   OutputControllerFlexCompleteBase::Finish();
}

bool PinscapePico::VerifySettings()
{
   if (m_number < 1 || m_number > 16)
   {
      Log::Write(StringExtensions::Build("PinscapePico {0} cannot be configured. Valid PinscapePico unit numbers are 1-16.", GetName()));
      return false;
   }

   return true;
}

void PinscapePico::ConnectToController()
{
   if (!VerifySettings())
      return;

   for (auto device : s_devices)
   {
      if (device->UnitNo() == m_number)
      {
         if (device->IsOpen() || device->Open())
         {
            m_dev = device;
            Log::Write(StringExtensions::Build("PinscapePico {0} connected successfully", GetName()));
            m_oldOutputValues.resize(GetNumberOfOutputs(), 0);
            return;
         }
      }
   }

   Log::Write(StringExtensions::Build("PinscapePico {0} could not connect to device", GetName()));
}

void PinscapePico::DisconnectFromController()
{
   if (m_dev)
   {
      m_dev->AllOff();
   }
}

class OutputListGroup
{
public:
   virtual ~OutputListGroup() = default;
   virtual int PacketCount() const = 0;
   virtual int PortCount() const = 0;
   virtual void Add(int port, int value, bool changed) = 0;
   virtual void Send(PinscapePico* psp) = 0;
};

class RandomAccessOutputList
{
public:
   std::vector<std::pair<int, int>> outputs;

   bool Add(int port, int value)
   {
      if (outputs.size() >= 30)
         return false;

      outputs.emplace_back(port, value);
      return true;
   }
};

class RandomAccessOutputListGroup : public OutputListGroup
{
private:
   std::vector<RandomAccessOutputList> list;
   RandomAccessOutputList* cur = nullptr;

public:
   int PacketCount() const override { return static_cast<int>(list.size()); }
   int PortCount() const override
   {
      int total = 0;
      for (const auto& l : list)
         total += static_cast<int>(l.outputs.size());
      return total;
   }

   void Add(int port, int value, bool changed) override
   {
      if (changed)
      {
         if (cur == nullptr)
            AddList();

         if (!cur->Add(port, value))
         {
            AddList();
            cur->Add(port, value);
         }
      }
   }

   void Send(PinscapePico* psp) override
   {
      for (const auto& l : list)
      {
         uint8_t buf[64] = { 0 };
         buf[0] = psp->m_dev->GetOutputReportId();
         buf[1] = 0x22;
         buf[2] = static_cast<uint8_t>(l.outputs.size());
         int idx = 3;
         for (const auto& output : l.outputs)
         {
            buf[idx++] = static_cast<uint8_t>(output.first + 1);
            buf[idx++] = static_cast<uint8_t>(output.second);
         }

         psp->m_dev->WriteUSB("SET OUTPUT PORTS", buf, 100);
      }
   }

private:
   void AddList()
   {
      list.emplace_back();
      cur = &list.back();
   }
};

class ContiguousOutputList
{
public:
   ContiguousOutputList(int firstPortNum)
      : firstPortNum(firstPortNum)
      , numToSend(0)
   {
   }
   int firstPortNum = 0;
   int numToSend = 0;
   std::vector<int> outputs;

   bool Add(int value, bool changed)
   {
      if (outputs.size() >= 60)
         return false;

      outputs.push_back(value);

      if (changed)
         numToSend = static_cast<int>(outputs.size());

      return true;
   }
};

class ContiguousOutputListGroup : public OutputListGroup
{
private:
   std::vector<ContiguousOutputList> list;
   ContiguousOutputList* cur = nullptr;

public:
   int PacketCount() const override { return static_cast<int>(list.size()); }
   int PortCount() const override
   {
      int total = 0;
      for (const auto& l : list)
         total += l.numToSend;
      return total;
   }

   void Add(int port, int value, bool changed) override
   {
      if (changed)
      {
         if (cur == nullptr)
            AddList(port);

         if (!cur->Add(value, changed))
         {
            AddList(port);
            cur->Add(value, changed);
         }
      }
      else if (cur != nullptr)
      {
         if (!cur->Add(value, changed))
            cur = nullptr;
      }
   }

   void Send(PinscapePico* psp) override
   {
      for (const auto& l : list)
      {
         uint8_t buf[64] = { 0 };
         buf[0] = psp->m_dev->GetOutputReportId();
         buf[1] = 0x21;
         buf[2] = static_cast<uint8_t>(l.numToSend);
         buf[3] = static_cast<uint8_t>(l.firstPortNum + 1);
         int idx = 4;
         for (const auto& output : l.outputs)
            buf[idx++] = static_cast<uint8_t>(output);

         psp->m_dev->WriteUSB("SET OUTPUT PORT BLOCK", buf, 100);
      }
   }

private:
   void AddList(int startingPort)
   {
      list.emplace_back(startingPort);
      cur = &list.back();
   }
};

void PinscapePico::UpdateOutputs(const std::vector<uint8_t>& newOutputValues)
{
   if (!m_dev)
      return;

   ContiguousOutputListGroup contiguousGroup;
   RandomAccessOutputListGroup randomGroup;
   int firstChangedIndex = -1;
   int lastChangedIndex = -1;

   for (int i = 0; i < GetNumberOfOutputs(); ++i)
   {
      int val = newOutputValues[i];
      bool changed = (newOutputValues[i] != m_oldOutputValues[i]);

      contiguousGroup.Add(i, val, changed);
      randomGroup.Add(i, val, changed);

      if (changed)
      {
         if (firstChangedIndex < 0)
            firstChangedIndex = i;
         lastChangedIndex = i;
      }
   }

   if (contiguousGroup.PacketCount() < randomGroup.PacketCount() || (contiguousGroup.PacketCount() <= randomGroup.PacketCount() && contiguousGroup.PortCount() < randomGroup.PortCount()))
      contiguousGroup.Send(this);
   else
      randomGroup.Send(this);

   if (firstChangedIndex >= 0)
   {
      std::copy(newOutputValues.begin() + firstChangedIndex, newOutputValues.begin() + lastChangedIndex + 1, m_oldOutputValues.begin() + firstChangedIndex);
   }
}

int PinscapePico::GetNumberOfConfiguredOutputs() { return GetNumberOfOutputs(); }

int PinscapePico::GetNumberOfOutputs() const
{
   if (m_dev)
      return m_dev->NumOutputs();
   return 128;
}

void PinscapePico::AllOff()
{
   if (m_dev && m_dev->IsOpen())
   {
      m_dev->AllOff();
   }
}

std::vector<PinscapePico::Device*> PinscapePico::GetAllDevices() { return s_devices; }

void PinscapePico::ClearDevices()
{
   for (Device* device : s_devices)
      delete device;

   s_devices.clear();
}

void PinscapePico::FindDevices()
{
   for (auto device : s_devices)
      delete device;
   s_devices.clear();

   struct hid_device_info* devs = hid_enumerate(0x0, 0x0);
   struct hid_device_info* cur = devs;

   while (cur)
   {
      if (cur->usage_page == 0x06 && cur->usage == 0x00)
      {
         hid_device* fp = hid_open_path(cur->path);
         if (fp)
         {
            std::string name = GetDeviceProductName(cur);
            if (name == "PinscapePico")
            {
               try
               {
                  Device* device = new Device(fp, cur->path, name, cur->vendor_id, cur->product_id, cur->release_number, 0, 4, 64, 4, 64);

                  if (device->UnitNo() > 0)
                  {
                     s_devices.push_back(device);
                     fp = nullptr;
                  }
                  else
                  {
                     delete device;
                  }
               }
               catch (...)
               {
                  if (fp)
                     hid_close(fp);
               }
            }
            else
            {
               hid_close(fp);
            }
         }
      }

      cur = cur->next;
   }

   hid_free_enumeration(devs);
}

std::string PinscapePico::GetDeviceProductName(hid_device_info* dev)
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
      return "";
#else
      std::wstring wstr(dev->product_string);
      std::string str(wstr.begin(), wstr.end());
      return str;
#endif
   }
   return "";
}

tinyxml2::XMLElement* PinscapePico::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerFlexCompleteBase::ToXml(doc);

   tinyxml2::XMLElement* numberElement = doc.NewElement("Number");
   numberElement->SetText(m_number);
   element->InsertEndChild(numberElement);

   if (m_minCommandIntervalMsSet)
   {
      tinyxml2::XMLElement* intervalElement = doc.NewElement("MinCommandIntervalMs");
      intervalElement->SetText(m_minCommandIntervalMs);
      element->InsertEndChild(intervalElement);
   }

   return element;
}

bool PinscapePico::FromXml(const tinyxml2::XMLElement* element)
{
   if (!OutputControllerFlexCompleteBase::FromXml(element))
      return false;

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

PinscapePico::Device::Device(hid_device* fp, const std::string& path, const std::string& name, uint16_t vendorID, uint16_t productID, int16_t productVersion, int protocolVersion,
   uint8_t inputReportId, uint8_t inputReportLength, uint8_t outputReportID, uint8_t outputReportLength)
   : m_fp(fp)
   , m_path(path)
   , m_name(name)
   , m_vendorID(vendorID)
   , m_productID(productID)
   , m_productVersion(productVersion)
   , m_protocolVersion(protocolVersion)
   , m_inputReportId(inputReportId)
   , m_inputReportLength(inputReportLength)
   , m_outputReportId(outputReportID)
   , m_outputReportLength(outputReportLength)
   , m_unitNo(0)
   , m_numOutputs(0)
   , m_unitName("NoName")
{
   DeviceRequest("QUERY DEVICE ID", 0x01);
   bool identified = false;
   for (int i = 0; i < 16; ++i)
   {
      uint8_t* buf = ReadUSB(100);
      if (buf != nullptr && buf[1] == 0x01)
      {
         m_unitNo = buf[2];

         char nameBuffer[33] = { 0 };
         memcpy(nameBuffer, &buf[3], 32);
         m_unitName = std::string(nameBuffer);

         size_t end = m_unitName.find_last_not_of('\0');
         if (end != std::string::npos)
            m_unitName = m_unitName.substr(0, end + 1);

         m_protocolVersion = ParseReportU16(buf, 35);

         std::stringstream ss;
         for (int j = 37; j < 45; ++j)
            ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(buf[j]);
         m_hardwareID = ss.str();

         m_numOutputs = ParseReportU16(buf, 45);
         m_plungerType = ParseReportU16(buf, 47);

         Log::Write(StringExtensions::Build("Pinscape Pico discovery found unit #{0} ({1}), {2} output ports, Pico hardware ID {3}, protocol version {4}",
            { std::to_string(m_unitNo), m_unitName, std::to_string(m_numOutputs), m_hardwareID, std::to_string(m_protocolVersion) }));

         SendWallClockTime();

         identified = true;
         break;
      }
   }

   if (!identified)
   {
      Log::Write(StringExtensions::Build("Pinscape Pico VID/PID {0:X4}/{1:X4} did not respond to identification query; this device's output port configuration is unknown, so no port "
                                         "updates will be sent to it during this session",
         { std::to_string(m_vendorID), std::to_string(m_productID) }));
   }
}

uint16_t PinscapePico::Device::ParseReportU16(uint8_t* buf, int index) { return static_cast<uint16_t>(buf[index]) | (static_cast<uint16_t>(buf[index + 1]) << 8); }

PinscapePico::Device::~Device()
{
   if (m_fp)
   {
      hid_close(m_fp);
      m_fp = nullptr;
   }
}

std::string PinscapePico::Device::ToString() const { return StringExtensions::Build("{0} (unit {1})", { m_name, std::to_string(m_unitNo) }); }

bool PinscapePico::Device::Open()
{
   if (!m_fp)
   {
      m_fp = hid_open_path(m_path.c_str());
   }
   return m_fp != nullptr;
}

uint8_t* PinscapePico::Device::ReadUSB(uint32_t timeoutMs)
{
   static uint8_t buf[64];
   memset(buf, 0, m_inputReportLength);
   buf[0] = m_inputReportId;

   uint32_t actual = 0;
   for (int tries = 0; tries < 3; ++tries)
   {
      int result = hid_read_timeout(m_fp, buf, m_inputReportLength, timeoutMs);
      if (result == m_inputReportLength)
         return buf;

      if (result < 0)
      {
         const wchar_t* error = hid_error(m_fp);
         std::string errorStr = "Unknown error";
         if (error)
         {
#ifdef _WIN32
            int size = WideCharToMultiByte(CP_UTF8, 0, error, -1, nullptr, 0, nullptr, nullptr);
            if (size > 0)
            {
               errorStr.resize(size - 1);
               WideCharToMultiByte(CP_UTF8, 0, error, -1, &errorStr[0], size, nullptr, nullptr);
            }
            else
            {
               errorStr = "USB error";
            }
#else
            std::wstring ws(error);
            errorStr = std::string(ws.begin(), ws.end());
#endif
         }
         Log::Write(StringExtensions::Build("Pinscape Pico: USB error reading from device: {0}", { errorStr }));
         continue;
      }

      if (result > 0 && result < m_inputReportLength)
      {
         Log::Write(StringExtensions::Build("Pinscape Pico: partial read - got {0} bytes, expected {1}", { std::to_string(result), std::to_string(m_inputReportLength) }));
         continue;
      }
   }

   return nullptr;
}

bool PinscapePico::Device::WriteUSB(const std::string& desc, uint8_t* buf, uint32_t timeoutMs)
{
   if (!m_fp || !buf)
      return false;

   std::string hexString;
   for (size_t i = 0; i < m_outputReportLength; ++i)
   {
      if (i > 0)
         hexString += ", ";
      char hexStr[4];
      snprintf(hexStr, sizeof(hexStr), "%02X", buf[i]);
      hexString += hexStr;
   }
   Log::Instrumentation("PinscapePico", StringExtensions::Build("PS Pico Write {0} : {1}", { desc, hexString }));

   uint32_t actual = 0;
   for (int tries = 0; tries < 3; ++tries)
   {
      int result = hid_write(m_fp, buf, m_outputReportLength);
      if (result == static_cast<int>(m_outputReportLength))
         return true;

      if (result < 0)
      {
         const wchar_t* error = hid_error(m_fp);
         std::string errorStr = "Unknown error";
         if (error)
         {
#ifdef _WIN32
            int size = WideCharToMultiByte(CP_UTF8, 0, error, -1, nullptr, 0, nullptr, nullptr);
            if (size > 0)
            {
               errorStr.resize(size - 1);
               WideCharToMultiByte(CP_UTF8, 0, error, -1, &errorStr[0], size, nullptr, nullptr);
            }
            else
            {
               errorStr = "USB error";
            }
#else
            std::wstring ws(error);
            errorStr = std::string(ws.begin(), ws.end());
#endif
         }
         Log::Write(StringExtensions::Build("PinscapePico USB write failed for {0}: {1}", { desc, errorStr }));
         continue;
      }
   }

   Log::Write(StringExtensions::Build("PinscapePico USB write failed for {0}: not all bytes sent", { desc }));
   return false;
}

void PinscapePico::Device::Close()
{
   if (m_fp)
   {
      hid_close(m_fp);
      m_fp = nullptr;
   }
}

void PinscapePico::Device::AllOff() { DeviceRequest("ALL PORTS OFF", 0x20); }

bool PinscapePico::Device::DeviceRequest(const std::string& desc, uint8_t commandId)
{
   uint8_t buf[64] = { 0 };
   buf[0] = m_outputReportId;
   buf[1] = commandId;
   return WriteUSB(desc, buf, 100);
}

void PinscapePico::Device::SendWallClockTime()
{
   auto now = std::chrono::system_clock::now();
   auto timeT = std::chrono::system_clock::to_time_t(now);
   auto tm = *std::localtime(&timeT);

   uint8_t timeBuf[64] = { 0 };
   timeBuf[0] = m_outputReportId;
   timeBuf[1] = 0x14;
   timeBuf[2] = static_cast<uint8_t>((tm.tm_year + 1900) & 0xFF);
   timeBuf[3] = static_cast<uint8_t>(((tm.tm_year + 1900) >> 8) & 0xFF);
   timeBuf[4] = static_cast<uint8_t>(tm.tm_mon + 1);
   timeBuf[5] = static_cast<uint8_t>(tm.tm_mday);
   timeBuf[6] = static_cast<uint8_t>(tm.tm_hour);
   timeBuf[7] = static_cast<uint8_t>(tm.tm_min);
   timeBuf[8] = static_cast<uint8_t>(tm.tm_sec);

   if (!WriteUSB("SET WALL CLOCK TIME", timeBuf, 100))
      Log::Write("Pinscape Pico: error setting wall clock time");
}


}