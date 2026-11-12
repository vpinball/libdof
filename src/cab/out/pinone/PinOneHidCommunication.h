#pragma once

#include <hidapi/hidapi.h>
#include <string>
#include <vector>
#include <cstdint>

namespace DOF
{

class PinOneHidDevice
{
public:
   PinOneHidDevice(hid_device* fp, const std::string& path, uint16_t vendorID, uint16_t productID);
   ~PinOneHidDevice();

   const std::string& GetPath() const { return m_path; }
   uint16_t GetVendorID() const { return m_vendorID; }
   uint16_t GetProductID() const { return m_productID; }

   void Close();
   bool WriteUSB(const std::vector<uint8_t>& payload);

private:
   bool TryReopenHandle();
   std::string GetLastHidErrMsg();

   hid_device* m_fp;
   std::string m_path;
   uint16_t m_vendorID;
   uint16_t m_productID;
};

class PinOneHidCommunication
{
public:
   static const uint16_t VendorID = 0x39F5;
   static const uint16_t ProductID = 0x9208;
   static const uint16_t UsagePage = 0xFF00;
   static const uint16_t Usage = 0x01;
   static const uint8_t ReportId = 6;
   static const int ReportSize = 64;

   static PinOneHidDevice* FindDevice();
   static std::vector<PinOneHidDevice*> FindDevices();
};

}
