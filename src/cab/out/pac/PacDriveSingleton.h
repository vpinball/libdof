#pragma once

#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <hidapi/hidapi.h>
#include <libusb-1.0/libusb.h>

namespace DOF
{

class PacDriveSingleton
{
public:
   enum DeviceType
   {
      Unknown,
      PacDrive,
      UHID,
      PacLED64,
      ServoStik,
      USBButton,
      NanoLED,
      IPACIO
   };

   enum FlashSpeed : uint8_t
   {
      AlwaysOn = 0,
      Seconds_2 = 1,
      Seconds_1 = 2,
      Seconds_0_5 = 3
   };

   static PacDriveSingleton& GetInstance();
   static void ClearDevices();
   static void ReacquireContext();

   std::vector<int> PacLed64GetIdList();
   int PacLed64GetIndexForDeviceId(int id);
   int PacLed64GetDeviceId(int index);
   bool PacLed64SetLEDIntensities(int index, const uint8_t* data);
   bool PacLed64SetLEDIntensity(int index, int port, uint8_t intensity);
   bool PacLed64SetLEDStates(int index, int group, uint8_t data);
   bool PacLed64SetLEDFadeTime(int index, uint8_t fadeTime);

   int PacDriveGetIndex();
   bool PacDriveUHIDSetLEDStates(int index, uint16_t data);

   std::vector<int> PacUIOGetIdList();
   int PacUIOGetIndexForDeviceId(int id);
   int PacUIOGetDeviceId(int index);

   int GetNumDevices() const { return m_numDevices; }
   DeviceType GetDeviceType(int index);
   std::string GetDevicePath(int index);

   void Initialize();
   void Shutdown();

private:
   PacDriveSingleton();
   ~PacDriveSingleton();

   PacDriveSingleton(const PacDriveSingleton&) = delete;
   PacDriveSingleton& operator=(const PacDriveSingleton&) = delete;

   struct DeviceInfo
   {
      DeviceType type;
      int deviceId;
      std::string path;
      uint16_t vendorId;
      uint16_t productId;
      std::string serial;
   };

   std::vector<DeviceInfo> m_devices;
   std::map<int, hid_device*> m_hidDevices;
   std::mutex m_hidDevicesMutex;
   int m_numDevices;

   libusb_context* m_usbContext;
   static PacDriveSingleton* s_pInstance;
   std::map<int, libusb_device_handle*> m_usbDevices;
   std::mutex m_usbDevicesMutex;

   void EnumerateDevices();
   bool IsPacLed64Device(uint16_t vendorId, uint16_t productId);
   bool IsPacDriveDevice(uint16_t vendorId, uint16_t productId);
   bool IsPacUIODevice(uint16_t vendorId, uint16_t productId);
   int ExtractPacLed64Id(uint16_t productId);
   int ExtractPacUIOId(uint16_t productId);

   hid_device* GetHidDevice(int index);
   void OpenHidDevice(int index);
   void CloseHidDevice(int index);

   libusb_device_handle* GetUsbDevice(int index);
   void OpenUsbDevice(int index);
   void CloseUsbDevice(int index);
};

}