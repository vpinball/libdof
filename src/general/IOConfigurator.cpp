#include "IOConfigurator.h"

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

void IOConfigurator::Initialize()
{
#ifdef __HIDAPI__
   hid_init();
#endif
}

void IOConfigurator::Shutdown()
{
#ifdef __LIBUSB__
   PacDriveSingleton::GetInstance().Shutdown();
#endif
#ifdef __HIDAPI__
   Pinscape::ClearDevices();
   PinscapePico::ClearDevices();
   hid_exit();
#endif
}

}
