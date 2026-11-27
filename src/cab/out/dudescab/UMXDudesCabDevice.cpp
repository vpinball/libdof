#include "UMXDudesCabDevice.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <stdexcept>

namespace DOF
{

UMXDudesCabDevice::UMXDudesCabDevice()
   : m_device(nullptr)
{
}

UMXDudesCabDevice::~UMXDudesCabDevice() { delete m_device; }

void UMXDudesCabDevice::Initialize()
{
   std::vector<uint8_t> answer;

   try
   {
      m_device->SendCommand(DudesCab::Device::HIDReportTypeMx::RT_UMXHANDSHAKE);
      answer = m_device->ReadUSB(static_cast<uint8_t>(DudesCab::Device::HIDReportTypeMx::RT_UMXHANDSHAKE));
      std::vector<uint8_t> data(answer.begin() + DudesCab::Device::hidCommandPrefixSize, answer.end());
      std::string handShake(data.begin(), data.end());
      handShake.erase(std::find(handShake.begin(), handShake.end(), '\0'), handShake.end());
      Log::Write(StringExtensions::Build("UMX Handshake : {0}", handShake));
      m_name = StringExtensions::Build("UMXDudesCab[{0}]", m_device->m_name);
   }
   catch (const std::exception& ex)
   {
      throw std::runtime_error(StringExtensions::Build("Exception during Handshake of UMXDudesCabDevice (answer size {0} bytes): {1}", std::to_string(answer.size()), ex.what()));
   }

   try
   {
      m_device->SendCommand(DudesCab::Device::HIDReportTypeMx::RT_MX_GETINFOS);
      answer = m_device->ReadUSB(static_cast<uint8_t>(DudesCab::Device::HIDReportTypeMx::RT_MX_GETINFOS));
      std::vector<uint8_t> data(answer.begin() + DudesCab::Device::hidCommandPrefixSize, answer.end());

      int index = 0;
      m_umxVersion.major = DudesCab::Device::ReadByte(data, index);
      m_umxVersion.minor = DudesCab::Device::ReadByte(data, index);
      m_umxVersion.build = DudesCab::Device::ReadByte(data, index);
      m_maxDataLines = DudesCab::Device::ReadByte(data, index);
      m_maxNbLeds = DudesCab::Device::ReadShort(data, index);
   }
   catch (const std::exception& ex)
   {
      throw std::runtime_error(StringExtensions::Build("Exception during GetInfos of UMXDudesCabDevice {0} : {1}", m_device->m_name, ex.what()));
   }

   try
   {
      m_device->SendCommand(DudesCab::Device::HIDReportTypeMx::RT_MX_GETCONFIG);
      answer = m_device->ReadUSB(static_cast<uint8_t>(DudesCab::Device::HIDReportTypeMx::RT_MX_GETCONFIG));
      std::vector<uint8_t> data(answer.begin() + DudesCab::Device::hidCommandPrefixSize, answer.end());

      m_totalLeds = 0;
      int index = 0;
      m_enabled = DudesCab::Device::ReadBool(data, index);
      m_ledChipset = static_cast<LedChipset>(DudesCab::Device::ReadByte(data, index));
      m_ledWizEquivalent = DudesCab::Device::ReadByte(data, index);
      m_testOnReset = static_cast<TestMode>(DudesCab::Device::ReadByte(data, index));
      m_testOnResetDuration = DudesCab::Device::ReadByte(data, index);
      m_testOnConnect = static_cast<TestMode>(DudesCab::Device::ReadByte(data, index));
      m_testOnConnectDuration = DudesCab::Device::ReadByte(data, index);
      m_testBrightness = DudesCab::Device::ReadByte(data, index);

      if (m_device->m_configVersion >= CompressionRatioVersion)
         m_compressionRatio = DudesCab::Device::ReadByte(data, index);

      uint8_t nbLedstrips = DudesCab::Device::ReadByte(data, index);
      m_ledStrips.clear();
      int curLedIndex = 0;

      for (int numStrip = 0; numStrip < nbLedstrips; numStrip++)
      {
         LedStripDescriptor ledstrip;
         ledstrip.m_name = DudesCab::Device::ReadString(data, index);
         ledstrip.m_width = DudesCab::Device::ReadShort(data, index);
         ledstrip.m_height = DudesCab::Device::ReadShort(data, index);
         ledstrip.m_firstLedIndex = curLedIndex;
         ledstrip.m_dofOutputNum = DudesCab::Device::ReadByte(data, index);
         ledstrip.m_fadeMode = static_cast<Curve::CurveTypeEnum>(DudesCab::Device::ReadByte(data, index));
         ledstrip.m_arrangement = static_cast<LedStripArrangementEnum>(DudesCab::Device::ReadByte(data, index));
         ledstrip.m_colorOrder = static_cast<RGBOrderEnum>(DudesCab::Device::ReadByte(data, index));
         ledstrip.m_brightness = DudesCab::Device::ReadByte(data, index);

         uint8_t nbSplits = DudesCab::Device::ReadByte(data, index);
         for (int numSplit = 0; numSplit < nbSplits; numSplit++)
         {
            LedStripSplit split;
            split.m_dataLine = DudesCab::Device::ReadByte(data, index);
            split.m_nbLeds = DudesCab::Device::ReadShort(data, index);
            ledstrip.m_splits.push_back(split);
         }

         curLedIndex += ledstrip.NbLeds();
         m_totalLeds += ledstrip.NbLeds();
         m_ledStrips.push_back(ledstrip);
      }
      ComputeNumOutputs();
   }
   catch (const std::exception& ex)
   {
      throw std::runtime_error(
         StringExtensions::Build("Exception during GetConfig of UMXDudesCabDevice {0} (answer size {1} bytes): {2}", { m_device->m_name, std::to_string(answer.size()), ex.what() }));
   }
}

void UMXDudesCabDevice::SendCommand(UMXCommand command, const std::vector<uint8_t>& parameters)
{
   auto dudeCommand = UMXToDudeCommand(command);
   if (command == UMXCommand::UMX_AllOff && m_device->NeedsReopen())
   {
      m_device->SendCommand(dudeCommand, parameters);
      if (!m_device->LastWriteFailed())
         return;
   }
   if (m_device->NeedsReopen() && !m_device->Reopen() && m_device->IsLost())
      throw std::runtime_error(StringExtensions::Build("{0} reports no serial number and cannot be opened again", m_name));
   m_device->SendCommand(dudeCommand, parameters);
   if (m_device->LastWriteFailed())
      throw std::runtime_error(StringExtensions::Build("Sending UMX command {0} failed", std::to_string(static_cast<int>(command))));
}

void UMXDudesCabDevice::WaitAck(uint8_t command) { }

void UMXDudesCabDevice::Release()
{
   if (m_device)
      m_device->Close();
}

DudesCab::Device::HIDReportTypeMx UMXDudesCabDevice::UMXToDudeCommand(UMXCommand command)
{
   switch (command)
   {
   case UMXCommand::UMX_SendStripsData: return DudesCab::Device::HIDReportTypeMx::RT_MX_OUTPUTS;
   case UMXCommand::UMX_StartTest: return DudesCab::Device::HIDReportTypeMx::RT_MX_RUNTEST;
   case UMXCommand::UMX_AllOff: return DudesCab::Device::HIDReportTypeMx::RT_MX_ALLOFF;
   case UMXCommand::UMX_Handshake: return DudesCab::Device::HIDReportTypeMx::RT_UMXHANDSHAKE;
   case UMXCommand::UMX_GetInfos: return DudesCab::Device::HIDReportTypeMx::RT_MX_GETINFOS;
   case UMXCommand::UMX_GetConfig: return DudesCab::Device::HIDReportTypeMx::RT_MX_GETCONFIG;
   default: throw std::runtime_error(StringExtensions::Build("Invalid UMX to Dude command remap [{0}]", std::to_string(static_cast<int>(command))));
   }
}

}
