#include "FTDI.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <cstring>

namespace DOF
{

FTDI::FTDI()
   : m_ftdiContext(nullptr)
   , m_isOpen(false)
{
   m_ftdiContext = ftdi_new();
   if (!m_ftdiContext)
   {
      Log::Exception("Failed to initialize FTDI context");
   }
}

FTDI::~FTDI()
{
   if (m_isOpen)
   {
      Close();
   }
   if (m_ftdiContext)
   {
      ftdi_free(m_ftdiContext);
   }
}

FTDI::FT_STATUS FTDI::ConvertLibftdiError(int error)
{
   switch (error)
   {
   case 0: return FT_OK;
   case -1: return FT_INVALID_HANDLE;
   case -2: return FT_DEVICE_NOT_FOUND;
   case -3: return FT_DEVICE_NOT_OPENED;
   case -4: return FT_IO_ERROR;
   case -5: return FT_INSUFFICIENT_RESOURCES;
   case -6: return FT_INVALID_PARAMETER;
   case -7: return FT_INVALID_BAUD_RATE;
   default: return FT_OTHER_ERROR;
   }
}

FTDI::FT_STATUS FTDI::GetNumberOfDevices(uint32_t& devcount)
{
   if (!m_ftdiContext)
      return FT_INVALID_HANDLE;

   struct ftdi_device_list* devlist = nullptr;
   int result = ftdi_usb_find_all(m_ftdiContext, &devlist, 0x0403, 0x6001);

   if (result < 0)
   {
      devcount = 0;
      return ConvertLibftdiError(result);
   }

   devcount = static_cast<uint32_t>(result);
   ftdi_list_free(&devlist);
   return FT_OK;
}

FTDI::FT_STATUS FTDI::GetDeviceInfoList(FT_DEVICE_INFO_NODE* devinfo, uint32_t& numdevs)
{
   if (!m_ftdiContext || !devinfo)
      return FT_INVALID_PARAMETER;

   struct ftdi_device_list* devlist = nullptr;
   int result = ftdi_usb_find_all(m_ftdiContext, &devlist, 0x0403, 0x6001);

   if (result < 0)
   {
      numdevs = 0;
      return ConvertLibftdiError(result);
   }

   const uint32_t capacity = numdevs;         // caller's buffer size (IN)
   numdevs = static_cast<uint32_t>(result);   // total devices found (OUT)

   struct ftdi_device_list* curdev = devlist;
   for (uint32_t i = 0; i < numdevs && i < capacity && curdev; ++i, curdev = curdev->next)
   {
      char manufacturer[256] = { 0 };
      char description[256] = { 0 };
      char serial[256] = { 0 };

      ftdi_usb_get_strings(m_ftdiContext, curdev->dev, manufacturer, 256, description, 256, serial, 256);

      devinfo[i].SerialNumber = serial;
      devinfo[i].Description = description;
      devinfo[i].Type = FT_DEVICE_232R;
      devinfo[i].ID = 0x0403 | (0x6001 << 16);
      devinfo[i].LocId = 0;
      devinfo[i].Flags = 0;
      devinfo[i].ftHandle = nullptr;
   }

   ftdi_list_free(&devlist);
   return FT_OK;
}

FTDI::FT_STATUS FTDI::OpenEx(const std::string& identifier, FT_OPEN_BY dwFlags)
{
   if (!m_ftdiContext)
      return FT_INVALID_HANDLE;

   if (m_isOpen)
      Close();

   int result;

   switch (dwFlags)
   {
   case FT_OPEN_BY_SERIAL_NUMBER: result = ftdi_usb_open_string(m_ftdiContext, ("s:" + identifier).c_str()); break;
   case FT_OPEN_BY_DESCRIPTION: result = ftdi_usb_open_string(m_ftdiContext, ("d:" + identifier).c_str()); break;
   default: return FT_INVALID_PARAMETER;
   }

   if (result < 0)
   {
      Log::Write(StringExtensions::Build("Failed to open FTDI device: {0}", ftdi_get_error_string(m_ftdiContext)));
      return ConvertLibftdiError(result);
   }

   m_isOpen = true;
   return FT_OK;
}

FTDI::FT_STATUS FTDI::Close()
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   int result = ftdi_usb_close(m_ftdiContext);
   m_isOpen = false;

   return ConvertLibftdiError(result);
}

FTDI::FT_STATUS FTDI::Read(uint8_t* ftBuffer, uint32_t dwBytesToRead, uint32_t& dwBytesReturned)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   if (!ftBuffer)
      return FT_INVALID_PARAMETER;

   int result = ftdi_read_data(m_ftdiContext, ftBuffer, static_cast<int>(dwBytesToRead));

   if (result < 0)
   {
      dwBytesReturned = 0;
      return ConvertLibftdiError(result);
   }

   dwBytesReturned = static_cast<uint32_t>(result);
   return FT_OK;
}

FTDI::FT_STATUS FTDI::Write(const uint8_t* ftBuffer, uint32_t dwBytesToWrite, uint32_t& dwBytesWritten)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   if (!ftBuffer)
      return FT_INVALID_PARAMETER;

   int result = ftdi_write_data(m_ftdiContext, const_cast<unsigned char*>(ftBuffer), static_cast<int>(dwBytesToWrite));

   if (result < 0)
   {
      dwBytesWritten = 0;
      return ConvertLibftdiError(result);
   }

   dwBytesWritten = static_cast<uint32_t>(result);
   return FT_OK;
}

FTDI::FT_STATUS FTDI::SetBitMode(uint8_t ucMask, uint8_t ucEnable)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   int result = ftdi_set_bitmode(m_ftdiContext, ucMask, ucEnable);
   return ConvertLibftdiError(result);
}

FTDI::FT_STATUS FTDI::GetBitMode(uint8_t& ucMode)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   unsigned char mode;
   int result = ftdi_read_pins(m_ftdiContext, &mode);
   ucMode = mode;
   return ConvertLibftdiError(result);
}

FTDI::FT_STATUS FTDI::SetBaudRate(uint32_t dwBaudRate)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   int result = ftdi_set_baudrate(m_ftdiContext, static_cast<int>(dwBaudRate));
   return ConvertLibftdiError(result);
}

FTDI::FT_STATUS FTDI::SetTimeouts(uint32_t dwReadTimeout, uint32_t dwWriteTimeout)
{
   if (!m_ftdiContext)
      return FT_INVALID_HANDLE;

   m_ftdiContext->usb_read_timeout = static_cast<int>(dwReadTimeout);
   m_ftdiContext->usb_write_timeout = static_cast<int>(dwWriteTimeout);
   return FT_OK;
}

FTDI::FT_STATUS FTDI::SetLatencyTimer(uint8_t ucLatency)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   int result = ftdi_set_latency_timer(m_ftdiContext, ucLatency);
   return ConvertLibftdiError(result);
}

FTDI::FT_STATUS FTDI::GetLatencyTimer(uint8_t& ucLatency)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   unsigned char latency;
   int result = ftdi_get_latency_timer(m_ftdiContext, &latency);
   ucLatency = latency;
   return ConvertLibftdiError(result);
}

FTDI::FT_STATUS FTDI::Purge(uint32_t dwMask)
{
   if (!m_ftdiContext || !m_isOpen)
      return FT_DEVICE_NOT_OPENED;

   int result = 0;

   if (dwMask & 0x01)
   {
      result = ftdi_tciflush(m_ftdiContext);
      if (result < 0)
         return ConvertLibftdiError(result);
   }

   if (dwMask & 0x02)
   {
      result = ftdi_tcoflush(m_ftdiContext);
      if (result < 0)
         return ConvertLibftdiError(result);
   }

   return FT_OK;
}

}