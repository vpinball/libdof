#include "../cab/out/ftdichip/FTDI.h"
#include "../Log.h"
#include <iostream>
#include <stdexcept>
#include <vector>

static std::vector<std::string> openedSerials;
static int openResult = 0;
static int closeCount = 0;
static int unexpectedCalls = 0;

static void Require(bool condition, const char* message)
{
   if (!condition)
      throw std::runtime_error(message);
}

namespace DOF
{
void Log::Write(const std::string&) { }
void Log::Exception(const std::string&) { }
}

extern "C"
{
ftdi_context* ftdi_new() { return new ftdi_context{}; }
void ftdi_free(ftdi_context* context) { delete context; }
int ftdi_usb_open_desc(ftdi_context*, int vendor, int product, const char* description, const char* serial)
{
   Require(vendor == 0x0403 && product == 0x6001, "Unexpected VID/PID");
   Require(description == nullptr, "Serial selection must not require a description");
   Require(serial != nullptr, "Serial selection must not fall back to the first device");
   openedSerials.emplace_back(serial);
   return openResult;
}
int ftdi_usb_open_string(ftdi_context*, const char*) { ++unexpectedCalls; return -11; }
int ftdi_usb_close(ftdi_context*) { ++closeCount; return 0; }
const char* ftdi_get_error_string(ftdi_context*) { return "simulated open failure"; }
int ftdi_usb_find_all(ftdi_context*, ftdi_device_list**, int, int) { ++unexpectedCalls; return -1; }
void ftdi_list_free(ftdi_device_list**) { ++unexpectedCalls; }
int ftdi_usb_get_strings(ftdi_context*, libusb_device*, char*, int, char*, int, char*, int) { ++unexpectedCalls; return -1; }
int ftdi_read_data(ftdi_context*, unsigned char*, int) { ++unexpectedCalls; return -1; }
int ftdi_write_data(ftdi_context*, const unsigned char*, int) { ++unexpectedCalls; return -1; }
int ftdi_set_bitmode(ftdi_context*, unsigned char, unsigned char) { ++unexpectedCalls; return -1; }
int ftdi_read_pins(ftdi_context*, unsigned char*) { ++unexpectedCalls; return -1; }
int ftdi_set_baudrate(ftdi_context*, int) { ++unexpectedCalls; return -1; }
int ftdi_set_latency_timer(ftdi_context*, unsigned char) { ++unexpectedCalls; return -1; }
int ftdi_get_latency_timer(ftdi_context*, unsigned char*) { ++unexpectedCalls; return -1; }
int ftdi_tciflush(ftdi_context*) { ++unexpectedCalls; return -1; }
int ftdi_tcoflush(ftdi_context*) { ++unexpectedCalls; return -1; }
}

int main()
{
   try
   {
      {
         DOF::FTDI first;
         DOF::FTDI second;
         Require(first.OpenEx("TEST_A", DOF::FTDI::FT_OPEN_BY_SERIAL_NUMBER) == DOF::FTDI::FT_OK, "First serial failed");
         Require(second.OpenEx("TEST_B", DOF::FTDI::FT_OPEN_BY_SERIAL_NUMBER) == DOF::FTDI::FT_OK, "Second serial failed");
         Require(first.IsOpen() && second.IsOpen(), "Both devices must remain open");
         Require(openedSerials == std::vector<std::string>{"TEST_A", "TEST_B"}, "Serials were transformed or reused");
         Require(closeCount == 0, "Opening a second device closed the first");
         Require(first.OpenEx("TEST_C", DOF::FTDI::FT_OPEN_BY_SERIAL_NUMBER) == DOF::FTDI::FT_OK, "Reopen failed");
         Require(closeCount == 1 && openedSerials.back() == "TEST_C", "Reopen did not close its previous handle");
      }
      Require(closeCount == 3, "Successful handles were not closed");
      {
         DOF::FTDI missing;
         openResult = -3;
         Require(missing.OpenEx("MISSING", DOF::FTDI::FT_OPEN_BY_SERIAL_NUMBER) != DOF::FTDI::FT_OK, "Failed open reported success");
         Require(!missing.IsOpen(), "Failed open retained an open state");
         Require(openedSerials.size() == 4 && openedSerials.back() == "MISSING", "Failed open retried a different device");
      }
      Require(closeCount == 3, "Failed handle was closed as if opened");
      Require(unexpectedCalls == 0, "Serial opening used string parsing, enumeration, or output operations");
      std::cout << "PASS: exact serial selection, concurrent devices, reopen, failure propagation and cleanup; no USB library linked\n";
      return 0;
   }
   catch (const std::exception& error)
   {
      std::cerr << "FAIL: " << error.what() << '\n';
      return 1;
   }
}
