#include "PinOneHidCommunication.h"
#include "../../../Log.h"
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace DOF
{

PinOneHidDevice::PinOneHidDevice(hid_device* fp, const std::string& path, uint16_t vendorID, uint16_t productID)
   : m_fp(fp)
   , m_path(path)
   , m_vendorID(vendorID)
   , m_productID(productID)
{
}

PinOneHidDevice::~PinOneHidDevice() { Close(); }

void PinOneHidDevice::Close()
{
   if (m_fp)
   {
      hid_close(m_fp);
      m_fp = nullptr;
   }
}

bool PinOneHidDevice::WriteUSB(const std::vector<uint8_t>& payload)
{
   std::vector<uint8_t> buf(PinOneHidCommunication::ReportSize, 0);
   buf[0] = PinOneHidCommunication::ReportId;
   std::copy(payload.begin(), payload.begin() + std::min(payload.size(), buf.size() - 1), buf.begin() + 1);

   for (int tries = 0; tries < 3; ++tries)
   {
      int actual = m_fp ? hid_write(m_fp, buf.data(), buf.size()) : -1;
      if (actual < 0)
      {
         std::string errMsg = GetLastHidErrMsg();
         if (TryReopenHandle())
            continue;

         Log::Error("PinOne Controller USB error sending request to device: " + errMsg);
         return false;
      }
      else if (actual != static_cast<int>(buf.size()))
      {
         Log::Error("PinOne Controller USB error sending request: not all bytes sent");
         return false;
      }
      else
      {
         return true;
      }
   }

   return false;
}

bool PinOneHidDevice::TryReopenHandle()
{
   Log::Error("PinOne Controller: invalid handle on write; trying to reopen handle");
   hid_device* fp2 = hid_open_path(m_path.c_str());

   if (fp2)
   {
      Close();
      m_fp = fp2;
      return true;
   }

   return false;
}

std::string PinOneHidDevice::GetLastHidErrMsg()
{
   const wchar_t* error = hid_error(m_fp);
   if (!error)
      return "Unknown error";

#ifdef _WIN32
   int size = WideCharToMultiByte(CP_UTF8, 0, error, -1, nullptr, 0, nullptr, nullptr);
   if (size <= 0)
      return "USB error";
   std::string errorStr(size - 1, '\0');
   WideCharToMultiByte(CP_UTF8, 0, error, -1, &errorStr[0], size, nullptr, nullptr);
   return errorStr;
#else
   std::wstring ws(error);
   return std::string(ws.begin(), ws.end());
#endif
}

PinOneHidDevice* PinOneHidCommunication::FindDevice()
{
   std::vector<PinOneHidDevice*> devices = FindDevices();
   if (devices.empty())
      return nullptr;

   for (size_t i = 1; i < devices.size(); i++)
      delete devices[i];

   return devices[0];
}

std::vector<PinOneHidDevice*> PinOneHidCommunication::FindDevices()
{
   std::vector<PinOneHidDevice*> devices;

   hid_device_info* devs = hid_enumerate(VendorID, ProductID);

   for (hid_device_info* cur = devs; cur; cur = cur->next)
   {
      if (cur->vendor_id != VendorID || cur->product_id != ProductID)
         continue;

      if (cur->usage_page != UsagePage || cur->usage != Usage)
         continue;

      hid_device* fp = hid_open_path(cur->path);
      if (!fp)
         continue;

      devices.push_back(new PinOneHidDevice(fp, cur->path, cur->vendor_id, cur->product_id));
   }

   hid_free_enumeration(devs);

   return devices;
}

}
