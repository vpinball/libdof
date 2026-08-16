#include "IOConfigurator.h"

#include "../Log.h"
#include "StringExtensions.h"

#ifdef __HIDAPI__
#include <hidapi/hidapi.h>
#include "../cab/out/ps/Pinscape.h"
#include "../cab/out/pspico/PinscapePico.h"
#endif

#ifdef __LIBUSB__
#include "../cab/out/pac/PacDriveSingleton.h"
#endif

#include <string>

namespace DOF
{

#ifdef __LIBUSB__
libusb_context* IOConfigurator::s_libusbContext = nullptr;
#endif

void IOConfigurator::Initialize()
{
#ifdef __HIDAPI__
   hid_init();
#endif
#ifdef __LIBUSB__
   if (s_libusbContext == nullptr)
   {
      int result = libusb_init(&s_libusbContext);
      if (result < 0)
      {
         Log::Exception(StringExtensions::Build("Failed to initialize libusb: {0}", std::to_string(result)));
         s_libusbContext = nullptr;
      }
   }

   PacDriveSingleton::ReacquireContext();
#endif
}

void IOConfigurator::Shutdown()
{
#ifdef __LIBUSB__
   // PacDriveSingleton is static, so its destructor runs at process exit - long after
   // libusb_exit() below frees the context its handles point into. Close them first.
   PacDriveSingleton::ClearDevices();

   if (s_libusbContext != nullptr)
   {
      libusb_exit(s_libusbContext);
      s_libusbContext = nullptr;
   }
#endif
#ifdef __HIDAPI__
   Pinscape::ClearDevices();
   PinscapePico::ClearDevices();
   hid_exit();
#endif
}

}
