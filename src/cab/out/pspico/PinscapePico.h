#pragma once

#include <hidapi/hidapi.h>

#include "../OutputControllerFlexCompleteBase.h"
#include "DOF/DOF.h"

namespace DOF
{

class PinscapePico : public OutputControllerFlexCompleteBase
{
   friend class ContiguousOutputListGroup;
   friend class RandomAccessOutputListGroup;
   friend class PinscapePicoAutoConfigurator;

public:
   class Device;
   static void Initialize();

   PinscapePico();
   PinscapePico(int number);
   ~PinscapePico();

   int GetNumber() { return m_number; }
   void SetNumber(int value);
   int GetMinCommandIntervalMs() const { return m_minCommandIntervalMs; }
   void SetMinCommandIntervalMs(int value);
   void Init(Cabinet* cabinet) override;
   void Finish() override;
   virtual bool VerifySettings() override;
   virtual void ConnectToController() override;
   virtual void DisconnectFromController() override;
   virtual void UpdateOutputs(const std::vector<uint8_t>& outputValues) override;

protected:
   virtual int GetNumberOfConfiguredOutputs() override;

public:
   int GetNumberOfOutputs() const;
   void AllOff();

   static void FindDevices();
   static void ClearDevices();
   static std::vector<Device*> GetAllDevices();

   class Device
   {
   public:
      Device(hid_device* fp, const std::string& path, const std::string& name, uint16_t vendorID, uint16_t productID, int16_t productVersion, int protocolVersion, uint8_t inputReportId,
         uint8_t inputReportLength, uint8_t outputReportID, uint8_t outputReportLength);
      ~Device();

      std::string ToString() const;
      int UnitNo() const { return m_unitNo; }
      int NumOutputs() const { return m_numOutputs; }
      const std::string& GetPath() const { return m_path; }
      const std::string& GetName() const { return m_name; }
      uint16_t GetVendorID() const { return m_vendorID; }
      uint16_t GetProductID() const { return m_productID; }
      int16_t GetProductVersion() const { return m_productVersion; }
      int GetProtocolVersion() const { return m_protocolVersion; }
      uint8_t GetInputReportId() const { return m_inputReportId; }
      uint8_t GetInputReportLength() const { return m_inputReportLength; }
      uint8_t GetOutputReportId() const { return m_outputReportId; }
      uint8_t GetOutputReportLength() const { return m_outputReportLength; }
      bool IsOpen() const { return m_fp != nullptr; }
      bool Open();
      uint8_t* ReadUSB(uint32_t timeoutMs);
      void Close();
      void AllOff();
      bool WriteUSB(const std::string& desc, uint8_t* buf, uint32_t timeoutMs);
      bool DeviceRequest(const std::string& desc, uint8_t commandId);

   private:
      hid_device* m_fp;
      std::string m_path;
      std::string m_name;
      uint16_t m_vendorID;
      uint16_t m_productID;
      int16_t m_productVersion;
      int m_protocolVersion;
      uint8_t m_inputReportId;
      uint8_t m_inputReportLength;
      uint8_t m_outputReportId;
      uint8_t m_outputReportLength;
      uint16_t m_unitNo;
      std::string m_unitName;
      std::string m_hardwareID;
      int m_plungerType;
      int m_numOutputs;

      uint16_t ParseReportU16(uint8_t* buf, int index);
      void SendWallClockTime();
   };

   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "PinscapePico"; }

private:
   static std::string GetDeviceProductName(hid_device_info* dev);

   int m_number;
   int m_minCommandIntervalMs;
   bool m_minCommandIntervalMsSet;
   std::vector<uint8_t> m_oldOutputValues;
   std::chrono::steady_clock::time_point m_lastUpdate;
   static std::vector<Device*> s_devices;
   Device* m_dev;
   bool m_cacheInvalid = true;
};

}