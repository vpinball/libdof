#pragma once

#include <hidapi/hidapi.h>

#include "../OutputControllerFlexCompleteBase.h"
#include "DOF/DOF.h"

namespace DOF
{

class Pinscape : public OutputControllerFlexCompleteBase
{
   friend class PinscapeAutoConfigurator;

public:
   static void Initialize();

   Pinscape();
   Pinscape(int number);
   ~Pinscape();

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
   void UpdateDelay();
   static void FindDevices();
   static void ClearDevices();
   static std::vector<void*> GetAllDevices();
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "Pinscape"; }

private:
   class Device
   {
   public:
      Device(hid_device* device, const std::string& path, const std::string& name, uint16_t vendorID, uint16_t productID, uint16_t version);
      ~Device();

      std::string ToString() const { return m_name + " (unit " + std::to_string(GetUnitNo()) + ")"; }
      int GetUnitNo() const { return m_unitNo; }
      int GetNumOutputs() const { return m_numOutputs; }
      bool IsLedWizEmulator(int unitNum);
      bool ReadUSB(uint8_t* buf);
      bool AllOff();
      bool SpecialRequest(uint8_t id);
      bool WriteUSB(uint8_t* buf);
      void UpdateOutputs(uint8_t* NewOutputValues);

   private:
      hid_device* m_device;
      std::string m_path;
      std::string m_name;
      uint16_t m_vendorID;
      uint16_t m_productID;
      uint16_t m_version;
      bool m_plungerEnabled;
      int m_numOutputs;
      int m_unitNo;
   };

   static std::string GetDeviceProductName(hid_device_info* dev);

   int m_number;
   int m_minCommandIntervalMs;
   bool m_minCommandIntervalMsSet;
   std::vector<uint8_t> m_oldOutputValues;
   bool m_cacheInvalid = true;
   std::chrono::steady_clock::time_point m_lastUpdate;
   static std::vector<Device*> s_devices;
   Device* m_dev;
};

}
