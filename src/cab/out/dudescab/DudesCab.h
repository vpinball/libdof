#pragma once

#include "../OutputControllerFlexCompleteBase.h"
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

#include <hidapi/hidapi.h>

namespace DOF
{

class Cabinet;

class DudesCab : public OutputControllerFlexCompleteBase
{
public:
   static const uint16_t VendorID = 0x2E8A;
   static const uint16_t ProductID = 0x106F;

   DudesCab();
   DudesCab(int number);
   virtual ~DudesCab();

   int GetNumber() const { return m_number; }
   void SetNumber(int value);

   int GetMinCommandIntervalMs() const { return m_minCommandIntervalMs; }
   void SetMinCommandIntervalMs(int value);

   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;

   virtual std::string GetXmlElementName() const override { return "DudesCab"; }
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;

   class Device
   {
   public:
      enum class RIDType : uint8_t
      {
         None = 0,
         RIDOutputs = 3,
         RIDOutputsMx = 5
      };

      enum class HIDCommonReportType : uint8_t
      {
         RT_HANDSHAKE = 1,
         RT_SETADMIN,
         RT_VERSION,
         RT_GETSTATUS,
         RT_FORCELOGLEVEL,
         RT_RESETCARD,
         RT_SETPROFILING,
         RT_GETPROFILING,
         RT_COMMONCOMMANDS_END = 99,
         RT_MAX,

         RT_OLD_VERSION = 2
      };

      enum class DudesCabLogLevel : uint8_t
      {
         None = 0,
         Errors,
         Warnings,
         Infos,
         Debug
      };

      enum class HIDReportType : uint8_t
      {
         RT_PWM_GETINFOS = static_cast<uint8_t>(HIDCommonReportType::RT_MAX),
         RT_PWM_ALLOFF,
         RT_PWM_OUTPUTS,

         RT_MAX,

         RT_PWM_OLD_GETINFOS = 3,
         RT_PWM_OLD_ALLOFF = 4,
         RT_PWM_OLD_OUTPUTS = 5,
      };

      enum class HIDReportTypeMx : uint8_t
      {
         RT_UMXHANDSHAKE = static_cast<uint8_t>(HIDCommonReportType::RT_MAX),
         RT_MX_GETINFOS,
         RT_MX_GETCONFIG,
         RT_MX_ALLOFF,
         RT_MX_OUTPUTS,
         RT_MX_RUNTEST,

         RT_MAX
      };

      struct Version
      {
         int major = 0;
         int minor = 0;
         int build = 0;

         Version() = default;
         Version(int maj, int min, int bld)
            : major(maj)
            , minor(min)
            , build(bld)
         {
         }

         bool operator<(const Version& other) const
         {
            if (major != other.major)
               return major < other.major;
            if (minor != other.minor)
               return minor < other.minor;
            return build < other.build;
         }
         bool operator>=(const Version& other) const { return !(*this < other); }
      };

      Device(RIDType rid, const std::string& path, const std::string& name, const std::string& serial, uint16_t vendorID, uint16_t productID, int16_t version);
      ~Device();

      std::string ToString() const;
      int UnitNo() const { return m_unitNo; }
      int NumOutputs() const { return m_numOutputs; }
      const std::string& GetSerial() const { return m_serial; }

      bool AllOff();
      bool Reopen();
      void Close();
      bool LastWriteFailed() const { return m_lastWriteFailed; }
      bool NeedsReopen() const { return m_needsReopen; }
      bool HasSerial() const { return !m_serial.empty() && m_serial != "<not available>"; }
      bool IsLost() const { return m_needsReopen && !HasSerial(); }
      void ReadPwmOutputsConfig();
      bool HasOutputEnabled(uint8_t extNum, uint8_t outputNum);
      bool SupportMx();

      void SendCommand(HIDCommonReportType command, const std::vector<uint8_t>& parameters = { });
      void SendCommand(HIDReportType command, const std::vector<uint8_t>& parameters = { });
      void SendCommand(HIDReportTypeMx command, const std::vector<uint8_t>& parameters = { });

      std::vector<uint8_t> ReadUSB(uint8_t command);

      static bool ReadBool(const std::vector<uint8_t>& data, int& index);
      static uint8_t ReadByte(const std::vector<uint8_t>& data, int& index);
      static int16_t ReadShort(const std::vector<uint8_t>& data, int& index);
      static int ReadLong(const std::vector<uint8_t>& data, int& index);
      static std::string ReadString(const std::vector<uint8_t>& data, int& index);

      static const int hidCommandPrefixSize = 5;

      std::string m_name;
      std::string m_devicename;
      RIDType m_deviceRid;
      int m_maxExtensions;
      uint8_t m_pwmMaxOutputsPerExtension;
      uint8_t m_pwmExtensionsMask;
      std::vector<int> m_pwmOutputsMask;
      Version m_firmwareVersion;
      uint8_t m_configVersion;

   private:
      void SendCommand(RIDType rid, uint8_t command, const std::vector<uint8_t>& parameters = { });
      std::vector<uint8_t> ReadUSB();
      bool WriteUSB(const std::vector<uint8_t>& data);

      HIDCommonReportType RemapCommonCommand(HIDCommonReportType command);
      HIDReportType RemapPwmCommand(HIDReportType command);
      uint8_t RemapIncomingCommand(uint8_t command);

      std::string m_path;
      std::string m_serial;
      uint16_t m_vendorID;
      uint16_t m_productID;
      int16_t m_version;
      int16_t m_unitNo;
      int m_numOutputs;

      hid_device* m_hidDevice;
      bool m_lastWriteFailed = false;
      bool m_needsReopen = false;

      static const Version s_minimalMxVersion;
      static const Version s_newProtocolVersion;
   };

   static std::vector<Device*> AllDevices();
   static void ClearDevices();

protected:
   virtual bool VerifySettings() override;
   virtual void UpdateOutputs(const std::vector<uint8_t>& newOutputValues) override;
   virtual void ConnectToController() override;
   virtual void DisconnectFromController() override;

private:
   std::mutex m_numberUpdateLocker;
   int m_number;
   int m_minCommandIntervalMs;
   bool m_minCommandIntervalMsSet;

   Device* m_dev;
   bool m_cacheInvalid = true;
   std::vector<uint8_t> m_oldOutputValues;
   std::vector<uint8_t> m_outputBuffer;

   std::chrono::steady_clock::time_point m_lastUpdate;
   void UpdateDelay();
   void Instrumentation(const std::string& message);

   static std::vector<Device*> FindDevices();
   static std::vector<Device*> s_devices;
   static bool s_discovered;
};

}
