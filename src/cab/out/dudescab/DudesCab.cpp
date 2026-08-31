#include "DudesCab.h"
#include "../../Cabinet.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../../general/MathExtensions.h"
#include "../adressableledstrip/UMXControllerAutoConfigurator.h"
#include "UMXDudesCabDevice.h"
#include <algorithm>
#include <cstring>
#include <thread>
#include <sstream>

#include <hidapi/hidapi.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef RT_VERSION
#endif

namespace DOF
{

std::vector<DudesCab::Device*> DudesCab::s_devices;

const DudesCab::Device::Version DudesCab::Device::s_minimalMxVersion(1, 9, 0);
const DudesCab::Device::Version DudesCab::Device::s_newProtocolVersion(1, 9, 0);

DudesCab::DudesCab()
   : m_number(-1)
   , m_minCommandIntervalMs(1)
   , m_minCommandIntervalMsSet(false)
   , m_dev(nullptr)
   , m_lastUpdate(std::chrono::steady_clock::now())
{
}

DudesCab::DudesCab(int number)
   : DudesCab()
{
   SetNumber(number);
}

DudesCab::~DudesCab() { }

void DudesCab::SetNumber(int value)
{
   if (!MathExtensions::IsBetween(value, 1, 5))
      throw std::runtime_error(StringExtensions::Build("DudesCab Unit Numbers must be between 1-5. The supplied number {0} is out of range.", std::to_string(value)));

   std::lock_guard<std::mutex> lock(m_numberUpdateLocker);

   if (m_number != value)
   {
      if (GetName().empty() || GetName() == StringExtensions::Build("DudesCab Controller {0:00}", std::to_string(m_number)))
         SetName(StringExtensions::Build("DudesCab Controller {0:00}", std::to_string(value)));

      m_number = value;

      auto it = std::find_if(s_devices.begin(), s_devices.end(), [this](Device* d) { return d->UnitNo() == m_number; });

      if (it != s_devices.end())
      {
         m_dev = *it;
         SetNumberOfOutputs(m_dev->NumOutputs());
         m_oldOutputValues.assign(GetNumberOfOutputs(), 255);
      }
   }
}

void DudesCab::SetMinCommandIntervalMs(int value)
{
   m_minCommandIntervalMs = MathExtensions::Limit(value, 0, 1000);
   m_minCommandIntervalMsSet = true;
}

void DudesCab::Init(Cabinet* cabinet)
{
   if (!m_minCommandIntervalMsSet && cabinet && cabinet->GetOwner()
      && cabinet->GetOwner()->GetConfigurationSettings().find("DudesCabDefaultMinCommandIntervalMs") != cabinet->GetOwner()->GetConfigurationSettings().end())
      m_minCommandIntervalMs = 1;

   OutputControllerFlexCompleteBase::Init(cabinet);
}

void DudesCab::Finish() { OutputControllerFlexCompleteBase::Finish(); }

bool DudesCab::VerifySettings() { return true; }

void DudesCab::Instrumentation(const std::string& message) { Log::Instrumentation("DudesCab", message); }

void DudesCab::UpdateOutputs(const std::vector<uint8_t>& newOutputValues)
{
   if (m_dev == nullptr)
      return;

   if (newOutputValues != m_oldOutputValues)
   {
      m_outputBuffer.clear();
      m_outputBuffer.push_back(0);
      int nbValuesToSend = 0;

      uint8_t extMask = 0;
      uint8_t oldExtMask = 0xFF;
      int outputMaskOffset = 0;
      uint16_t outputMask = 0;

      for (int numDofOutput = 0; numDofOutput < static_cast<int>(newOutputValues.size()); numDofOutput++)
      {
         if (m_oldOutputValues.size() <= static_cast<size_t>(numDofOutput) || newOutputValues[numDofOutput] != m_oldOutputValues[numDofOutput])
         {
            uint8_t extNum = static_cast<uint8_t>(numDofOutput / m_dev->m_pwmMaxOutputsPerExtension);
            uint8_t outputNum = static_cast<uint8_t>(numDofOutput % m_dev->m_pwmMaxOutputsPerExtension);

            if (m_dev->HasOutputEnabled(extNum, outputNum))
            {
               Instrumentation(StringExtensions::Build("Prepare Dof Value to send : DOF #{0} {1} => {2}, Extension #{3}, Output #{4}",
                  { std::to_string(numDofOutput), m_oldOutputValues.size() > static_cast<size_t>(numDofOutput) ? std::to_string(m_oldOutputValues[numDofOutput]) : "0",
                     std::to_string(newOutputValues[numDofOutput]), std::to_string(extNum), std::to_string(outputNum) }));

               extMask |= (1 << extNum);
               if (oldExtMask != extMask)
               {
                  oldExtMask = extMask;
                  if (outputMask != 0)
                  {
                     m_outputBuffer[outputMaskOffset] = static_cast<uint8_t>(outputMask & 0xFF);
                     m_outputBuffer[outputMaskOffset + 1] = static_cast<uint8_t>((outputMask >> 8) & 0xFF);
                     Instrumentation(StringExtensions::Build("        Changed OutputMask 0x{0:X4}", std::to_string(outputMask)));
                     outputMask = 0;
                  }
                  Instrumentation(StringExtensions::Build("    Extension {0} has changes", std::to_string(extNum)));
                  outputMaskOffset = static_cast<int>(m_outputBuffer.size());
                  m_outputBuffer.push_back(0);
                  m_outputBuffer.push_back(0);
               }

               outputMask |= (1 << outputNum);
               m_outputBuffer.push_back(newOutputValues[numDofOutput]);
               nbValuesToSend++;
            }
            else
            {
               Instrumentation(StringExtensions::Build("You are sending Pwm updates to a disabled output or a missing extension (ext: {0}, output: {1}), it could be a wrong configuration "
                                                       "or a missmatch with your config on dofconfigtool site.",
                  std::to_string(extNum + 1), std::to_string(outputNum + 1)));
            }
         }
      }

      if (outputMask != 0)
      {
         m_outputBuffer[outputMaskOffset] = static_cast<uint8_t>(outputMask & 0xFF);
         m_outputBuffer[outputMaskOffset + 1] = static_cast<uint8_t>((outputMask >> 8) & 0xFF);
         Instrumentation(StringExtensions::Build("        Changed OutputMask 0x{0:X4}", std::to_string(outputMask)));
      }
      m_outputBuffer[0] = extMask;
      Instrumentation(StringExtensions::Build("    ExtenstionMask 0x{0:X2}", std::to_string(m_outputBuffer[0])));

      Instrumentation(StringExtensions::Build("{0} Dof Values to send to Dude's cab", std::to_string(nbValuesToSend)));

      m_dev->SendCommand(Device::HIDReportType::RT_PWM_OUTPUTS, m_outputBuffer);

      m_oldOutputValues = newOutputValues;
   }
}

void DudesCab::UpdateDelay()
{
   auto now = std::chrono::steady_clock::now();
   auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdate).count();

   if (elapsed < m_minCommandIntervalMs)
      std::this_thread::sleep_for(std::chrono::milliseconds(m_minCommandIntervalMs - elapsed));
   m_lastUpdate = std::chrono::steady_clock::now();
}

void DudesCab::ConnectToController() { DisconnectFromController(); }

void DudesCab::DisconnectFromController()
{
   if (m_inUseState == InUseState::Running && m_dev)
   {
      m_dev->AllOff();
      m_dev = nullptr;
   }
}

std::vector<DudesCab::Device*> DudesCab::AllDevices()
{
   if (s_devices.empty())
      s_devices = FindDevices();
   return s_devices;
}

void DudesCab::ClearDevices()
{
   for (Device* device : s_devices)
      delete device;

   s_devices.clear();
}

std::vector<DudesCab::Device*> DudesCab::FindDevices()
{
   UMXControllerAutoConfigurator::RemoveAllUMXDevices();

   std::vector<Device*> dudedevices;
   std::vector<Device*> devices;

   struct hid_device_info* devs = hid_enumerate(VendorID, ProductID);
   struct hid_device_info* cur_dev = devs;

   while (cur_dev)
   {
      std::string productName;
      if (cur_dev->product_string)
      {
#ifdef _WIN32
         int size = WideCharToMultiByte(CP_UTF8, 0, cur_dev->product_string, -1, nullptr, 0, nullptr, nullptr);
         if (size > 0)
         {
            productName.resize(size - 1);
            WideCharToMultiByte(CP_UTF8, 0, cur_dev->product_string, -1, &productName[0], size, nullptr, nullptr);
         }
         else
            productName = "<not available>";
#else
         wchar_t* wstr = cur_dev->product_string;
         while (*wstr)
            productName += static_cast<char>(*wstr++);
#endif
      }

      std::string serialNumber;
      if (cur_dev->serial_number)
      {
#ifdef _WIN32
         int size = WideCharToMultiByte(CP_UTF8, 0, cur_dev->serial_number, -1, nullptr, 0, nullptr, nullptr);
         if (size > 0)
         {
            serialNumber.resize(size - 1);
            WideCharToMultiByte(CP_UTF8, 0, cur_dev->serial_number, -1, &serialNumber[0], size, nullptr, nullptr);
         }
         else
            serialNumber = "<not available>";
#else
         wchar_t* wserial = cur_dev->serial_number;
         while (*wserial)
            serialNumber += static_cast<char>(*wserial++);
#endif
      }
      else
         serialNumber = "<not available>";

      bool isDude = (cur_dev->vendor_id == VendorID && cur_dev->product_id == ProductID);
      if (isDude && !productName.empty())
      {
         Device::RIDType deviceRid = Device::RIDType::None;

         if (productName == "DudesCab Outputs")
            deviceRid = Device::RIDType::RIDOutputs;
         else if (productName == "DudesCab Outputs MX")
            deviceRid = Device::RIDType::RIDOutputsMx;
#if defined(__linux__) || defined(__APPLE__)
         else if (productName == "DudesCab" && cur_dev->interface_number == static_cast<int>(Device::RIDType::RIDOutputs))
            deviceRid = Device::RIDType::RIDOutputs;
#endif

         if (deviceRid != Device::RIDType::None)
         {
            bool alreadyEnumerated = false;
            for (Device* existingDevice : dudedevices)
            {
               if (existingDevice->m_deviceRid == deviceRid && existingDevice->GetSerial() == serialNumber && existingDevice->m_devicename == productName)
               {
                  alreadyEnumerated = true;
                  break;
               }
            }

            if (!alreadyEnumerated)
            {
               Device* newDevice = new Device(deviceRid, cur_dev->path, productName, serialNumber, cur_dev->vendor_id, cur_dev->product_id, cur_dev->release_number);
               dudedevices.push_back(newDevice);

               if (deviceRid == Device::RIDType::RIDOutputs)
               {
                  newDevice->ReadPwmOutputsConfig();
                  devices.push_back(newDevice);
               }
               else if (deviceRid == Device::RIDType::RIDOutputsMx)
               {
                  if (newDevice->SupportMx())
                  {
                     UMXDudesCabDevice* dudeUMX = new UMXDudesCabDevice();
                     dudeUMX->SetDevice(newDevice);
                     UMXControllerAutoConfigurator::AddUMXDevice(dudeUMX);
                  }
               }
            }
         }
      }
      cur_dev = cur_dev->next;
   }

   hid_free_enumeration(devs);

   return devices;
}

DudesCab::Device::Device(RIDType rid, const std::string& path, const std::string& name, const std::string& serial, uint16_t vendorID, uint16_t productID, int16_t version)
   : m_deviceRid(rid)
   , m_path(path)
   , m_devicename(name)
   , m_serial(serial)
   , m_vendorID(vendorID)
   , m_productID(productID)
   , m_version(version)
   , m_unitNo(0)
   , m_numOutputs(128)
   , m_maxExtensions(0)
   , m_pwmMaxOutputsPerExtension(0)
   , m_pwmExtensionsMask(0)
   , m_configVersion(0)
   , m_hidDevice(nullptr)
{
   m_hidDevice = hid_open_path(path.c_str());
   if (!m_hidDevice)
      return;

   std::vector<uint8_t> answer;

   SendCommand(HIDCommonReportType::RT_HANDSHAKE);
   answer = ReadUSB(static_cast<uint8_t>(HIDCommonReportType::RT_HANDSHAKE));
   if (answer.size() > hidCommandPrefixSize)
   {
      std::string handShake(answer.begin() + hidCommandPrefixSize, answer.end());
      handShake.erase(std::find(handShake.begin(), handShake.end(), '\0'), handShake.end());

      std::vector<std::string> splits;
      std::istringstream stream(handShake);
      std::string token;
      while (std::getline(stream, token, '|'))
         splits.push_back(token);

      switch (splits.size())
      {
      case 1: Log::Warning("Old Dude's Cab handshake, you should update your firmware"); break;

      case 2:
         Log::Warning("Old Dude's Cab handshake, you should update your firmware");
         m_name = splits[0];
         handShake = splits[1];
         break;

      case 3:
      default:
         m_name = splits[0];
         handShake = splits[1];
         {
            std::vector<std::string> numbers;
            std::istringstream vstream(splits[2]);
            std::string vtoken;
            while (std::getline(vstream, vtoken, '.'))
               numbers.push_back(vtoken);
            if (numbers.size() == 3)
               m_firmwareVersion = Version(std::stoi(numbers[0]), std::stoi(numbers[1]), std::stoi(numbers[2]));
         }
         break;
      }
      Log::Write(StringExtensions::Build("{0} says : {1}", m_name, handShake));
   }

   SendCommand(HIDCommonReportType::RT_VERSION);
   answer = ReadUSB(static_cast<uint8_t>(HIDCommonReportType::RT_VERSION));
   if (answer.size() > hidCommandPrefixSize)
   {
      std::vector<uint8_t> info(answer.begin() + hidCommandPrefixSize, answer.end());
      if (info.size() >= 5)
      {
         Log::Write(StringExtensions::Build("DudesCab Controller Informations : Device [{0},RID:{1}] Name [{2}], v{3}.{4}.{5}, unit #{6}, Max extensions {7}",
            { m_devicename, std::to_string(static_cast<int>(m_deviceRid)), m_name, std::to_string(info[0]), std::to_string(info[1]), std::to_string(info[2]), std::to_string(info[3]),
               std::to_string(info[4]) }));
         m_unitNo = info[3];
         m_maxExtensions = info[4];
         m_firmwareVersion = Version(info[0], info[1], info[2]);
         if (info.size() >= 6)
            m_configVersion = info[5];
      }
   }
}

DudesCab::Device::~Device()
{
   if (m_hidDevice)
      hid_close(m_hidDevice);
}

std::string DudesCab::Device::ToString() const { return StringExtensions::Build("{0} (name: {1} unit:{2})", m_devicename, m_name, std::to_string(m_unitNo)); }

void DudesCab::Device::ReadPwmOutputsConfig()
{
   m_numOutputs = 128;

   SendCommand(HIDReportType::RT_PWM_GETINFOS);
   std::vector<uint8_t> answer = ReadUSB(static_cast<uint8_t>(HIDReportType::RT_PWM_GETINFOS));
   if (answer.size() <= hidCommandPrefixSize)
      return;

   uint8_t answersize = answer[hidCommandPrefixSize - 1];
   std::vector<uint8_t> data(answer.begin() + hidCommandPrefixSize, answer.end());
   if (data.size() < 2)
      return;

   m_pwmMaxOutputsPerExtension = data[0];
   m_pwmExtensionsMask = data[1];
   m_pwmOutputsMask.assign(m_maxExtensions, 0);
   Log::Write(StringExtensions::Build(
      "    Pwm Informations : Max outputs per extensions {0}, Extension Mask 0x{1}", std::to_string(m_pwmMaxOutputsPerExtension), std::to_string(m_pwmExtensionsMask)));

   if (answersize > 2 && data.size() >= 4)
   {
      uint8_t nbMasks = data[2];
      uint8_t maskSize = data[3];
      std::vector<uint16_t> masks(nbMasks);
      for (int mask = 0; mask < nbMasks; mask++)
         masks[mask] = static_cast<uint16_t>(data[4 + (2 * mask)] + (data[4 + (2 * mask) + 1] << 8));

      int curMask = 0;
      m_numOutputs = 0;
      for (int ext = 0; ext < m_maxExtensions; ext++)
      {
         if ((m_pwmExtensionsMask & (1 << ext)) != 0)
         {
            m_pwmOutputsMask[ext] = masks[curMask];
            for (int output = 0; output < m_pwmMaxOutputsPerExtension; output++)
            {
               if ((masks[curMask] & (1 << output)) != 0)
                  m_numOutputs = std::max(m_numOutputs, (ext * m_pwmMaxOutputsPerExtension) + output + 1);
            }
            curMask++;
         }
         else
            m_pwmOutputsMask[ext] = 0;
      }
      Log::Write(StringExtensions::Build("    Output configuration received, highest configured output is #{0}", std::to_string(m_numOutputs)));
   }
   else
   {
      Log::Warning(StringExtensions::Build("No output configuration received, {0} will be used, you should update your DudesCab firmware", std::to_string(m_numOutputs)));
   }
}

bool DudesCab::Device::HasOutputEnabled(uint8_t extNum, uint8_t outputNum)
{
   bool hasExtension = ((1 << extNum) & m_pwmExtensionsMask) != 0;
   bool hasOutput = hasExtension && (m_pwmOutputsMask[extNum] & (1 << outputNum)) != 0;
   return hasOutput;
}

bool DudesCab::Device::SupportMx() { return m_firmwareVersion >= s_minimalMxVersion; }

void DudesCab::Device::AllOff() { SendCommand(HIDReportType::RT_PWM_ALLOFF); }

std::vector<uint8_t> DudesCab::Device::ReadUSB(uint8_t command)
{
   uint8_t remapCommand = RemapIncomingCommand(command);
   std::vector<uint8_t> answer;
   try
   {
      while (answer.size() < 2 || answer[1] != remapCommand)
         answer = ReadUSB();
   }
   catch (const std::exception& ex)
   {
      throw std::runtime_error(StringExtensions::Build("Exception during answer retrieval for command {0} (remap:{1}) on DudesCabDevice {2} RID {3}: {4}",
         { std::to_string(command), std::to_string(remapCommand), m_name, std::to_string(static_cast<int>(m_deviceRid)), ex.what() }));
   }
   return answer;
}

std::vector<uint8_t> DudesCab::Device::ReadUSB()
{
   std::vector<uint8_t> incomingData;
   uint8_t receivedCommand = 0;
   auto startRead = std::chrono::steady_clock::now();

   while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startRead).count() < 10)
   {
      uint8_t buf[65];
      int bytesRead = hid_read_timeout(m_hidDevice, buf, sizeof(buf), 1000);
      if (bytesRead > 0)
      {
         uint8_t rid = buf[0];
         if (rid == static_cast<uint8_t>(m_deviceRid))
         {
            uint8_t numpart = buf[2];
            if (numpart == 0)
               receivedCommand = buf[1];
            uint8_t nbparts = buf[3];
            uint8_t received = buf[4];

            if (receivedCommand == buf[1])
            {
               if (numpart == 0)
                  incomingData.assign(buf, buf + received + hidCommandPrefixSize);
               else
                  incomingData.insert(incomingData.end(), buf + hidCommandPrefixSize, buf + hidCommandPrefixSize + received);

               if (numpart == nbparts - 1)
                  return incomingData;
            }
         }
      }
   }

   Log::Error("DudesCab Controller USB error reading from device: timed out");
   return { };
}

bool DudesCab::Device::WriteUSB(const std::vector<uint8_t>& data)
{
   if (m_hidDevice && !data.empty())
   {
      int result = hid_write(m_hidDevice, data.data(), data.size());
      return result >= 0;
   }
   return false;
}

void DudesCab::Device::SendCommand(HIDCommonReportType command, const std::vector<uint8_t>& parameters)
{
   SendCommand(m_deviceRid, static_cast<uint8_t>(RemapCommonCommand(command)), parameters);
}

void DudesCab::Device::SendCommand(HIDReportType command, const std::vector<uint8_t>& parameters) { SendCommand(m_deviceRid, static_cast<uint8_t>(RemapPwmCommand(command)), parameters); }

void DudesCab::Device::SendCommand(HIDReportTypeMx command, const std::vector<uint8_t>& parameters)
{
   if (SupportMx())
      SendCommand(m_deviceRid, static_cast<uint8_t>(command), parameters);
}

void DudesCab::Device::SendCommand(RIDType rid, uint8_t command, const std::vector<uint8_t>& parameters)
{
   if (!m_hidDevice)
      return;

   if (rid == RIDType::RIDOutputs)
      Log::Instrumentation("DudesCab", StringExtensions::Build("DudesCab SendCommand: {0}", std::to_string(static_cast<int>(command))));
   else
      Log::Instrumentation("DudesCab,Mx", StringExtensions::Build("DudesCab SendCommand: {0}", std::to_string(static_cast<int>(command))));

   std::vector<uint8_t> data = parameters;

   uint8_t bufferOffset = 5;
   uint8_t partSize = 60;
   uint8_t nbParts = static_cast<uint8_t>((data.size() / partSize) + 1);

   for (uint8_t i = 0; i < nbParts; i++)
   {
      std::vector<uint8_t> sendData(bufferOffset);
      sendData[0] = static_cast<uint8_t>(rid);
      sendData[1] = command;
      sendData[2] = i;
      sendData[3] = nbParts;

      size_t remainingData = data.size() - (i * partSize);
      uint8_t toSend = static_cast<uint8_t>(std::min(static_cast<size_t>(partSize), remainingData));
      sendData[4] = toSend;

      if (toSend > 0)
      {
         size_t startPos = i * partSize;
         sendData.insert(sendData.end(), data.begin() + startPos, data.begin() + startPos + toSend);
      }

      WriteUSB(sendData);
   }
}

DudesCab::Device::HIDCommonReportType DudesCab::Device::RemapCommonCommand(HIDCommonReportType command)
{
   if (m_firmwareVersion < s_newProtocolVersion)
   {
      switch (command)
      {
      case HIDCommonReportType::RT_VERSION: return HIDCommonReportType::RT_OLD_VERSION;
      default: break;
      }
   }
   return command;
}

DudesCab::Device::HIDReportType DudesCab::Device::RemapPwmCommand(HIDReportType command)
{
   if (m_firmwareVersion < s_newProtocolVersion)
   {
      switch (command)
      {
      case HIDReportType::RT_PWM_GETINFOS: return HIDReportType::RT_PWM_OLD_GETINFOS;
      case HIDReportType::RT_PWM_ALLOFF: return HIDReportType::RT_PWM_OLD_ALLOFF;
      case HIDReportType::RT_PWM_OUTPUTS: return HIDReportType::RT_PWM_OLD_OUTPUTS;
      default: break;
      }
   }
   return command;
}

uint8_t DudesCab::Device::RemapIncomingCommand(uint8_t command)
{
   if (m_firmwareVersion < s_newProtocolVersion)
   {
      switch (command)
      {
      case static_cast<uint8_t>(HIDCommonReportType::RT_VERSION): return static_cast<uint8_t>(HIDCommonReportType::RT_OLD_VERSION);
      case static_cast<uint8_t>(HIDReportType::RT_PWM_GETINFOS): return static_cast<uint8_t>(HIDReportType::RT_PWM_OLD_GETINFOS);
      case static_cast<uint8_t>(HIDReportType::RT_PWM_ALLOFF): return static_cast<uint8_t>(HIDReportType::RT_PWM_OLD_ALLOFF);
      case static_cast<uint8_t>(HIDReportType::RT_PWM_OUTPUTS): return static_cast<uint8_t>(HIDReportType::RT_PWM_OLD_OUTPUTS);
      default: break;
      }
   }
   return command;
}

bool DudesCab::Device::ReadBool(const std::vector<uint8_t>& data, int& index) { return data[index++] > 0; }
uint8_t DudesCab::Device::ReadByte(const std::vector<uint8_t>& data, int& index) { return data[index++]; }
int16_t DudesCab::Device::ReadShort(const std::vector<uint8_t>& data, int& index)
{
   int16_t val = static_cast<int16_t>(data[index] | (data[index + 1] << 8));
   index += 2;
   return val;
}
int DudesCab::Device::ReadLong(const std::vector<uint8_t>& data, int& index)
{
   int val = data[index] | (data[index + 1] << 8) | (data[index + 2] << 16) | (data[index + 3] << 24);
   index += 4;
   return val;
}
std::string DudesCab::Device::ReadString(const std::vector<uint8_t>& data, int& index)
{
   std::string strRead;
   uint8_t len = data[index++];
   if (len > 0)
   {
      for (int i = 0; i < len; i++)
         strRead += static_cast<char>(data[index++]);
   }
   return strRead;
}

bool DudesCab::FromXml(const tinyxml2::XMLElement* element)
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

tinyxml2::XMLElement* DudesCab::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerFlexCompleteBase::ToXml(doc);

   tinyxml2::XMLElement* numberElement = doc.NewElement("Number");
   numberElement->SetText(m_number);
   element->InsertEndChild(numberElement);

   tinyxml2::XMLElement* intervalElement = doc.NewElement("MinCommandIntervalMs");
   intervalElement->SetText(m_minCommandIntervalMs);
   element->InsertEndChild(intervalElement);

   return element;
}

}
