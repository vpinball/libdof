#pragma once

#include "../adressableledstrip/UMXDevice.h"
#include "DudesCab.h"

namespace DOF
{

class UMXDudesCabDevice : public UMXDevice
{
public:
   static const uint8_t CompressionRatioVersion = 7;

   UMXDudesCabDevice();
   virtual ~UMXDudesCabDevice();

   void Initialize() override;
   void SendCommand(UMXCommand command, const std::vector<uint8_t>& parameters = { }) override;
   void WaitAck(uint8_t command) override;
   bool IsLost() const override { return m_device && m_device->IsLost(); }
   void Release() override;

   DudesCab::Device* GetDevice() const { return m_device; }
   void SetDevice(DudesCab::Device* device) { m_device = device; }

private:
   DudesCab::Device::HIDReportTypeMx UMXToDudeCommand(UMXCommand command);
   DudesCab::Device* m_device;
};

}
