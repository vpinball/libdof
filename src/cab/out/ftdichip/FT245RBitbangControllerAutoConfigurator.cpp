#include "FT245RBitbangControllerAutoConfigurator.h"
#include "FT245RBitbangController.h"
#include "FTDI.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../../toys/ToyList.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <vector>

namespace DOF
{

void FT245RBitbangControllerAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   Log::Write("FTDI auto-configuration starting");

   FTDI* dummyFTDI = new FTDI();
   uint32_t amountDevices = 0;
   std::vector<DeviceInfo> deviceList;

   FTDI::FT_STATUS status = dummyFTDI->GetNumberOfDevices(amountDevices);
   delete dummyFTDI;

   Log::Write(StringExtensions::Build("FTDI device scan found {0} devices", std::to_string(amountDevices)));

   if (status != FTDI::FT_OK || amountDevices == 0)
   {
      return;
   }

   // Fill one properly-sized buffer in a single call and index each device by i,
   // like the C# OpenByIndex(i) loop. The previous code passed a 1-element buffer
   // per iteration (GetDeviceInfoList writes one node per device -> overflow with
   // 2+ boards) and never used i (so every controller got device[0]'s serial).
   std::vector<FTDI::FT_DEVICE_INFO_NODE> devInfos(amountDevices);
   uint32_t numDevs = amountDevices;
   FTDI* connectFTDI = new FTDI();
   status = connectFTDI->GetDeviceInfoList(devInfos.data(), numDevs);
   delete connectFTDI;

   if (status == FTDI::FT_OK)
   {
      for (uint32_t i = 0; i < numDevs && i < devInfos.size(); i++)
         deviceList.emplace_back(devInfos[i].SerialNumber, devInfos[i].Description);
   }

   for (int deviceIndex = 0; deviceIndex < static_cast<int>(deviceList.size()); deviceIndex++)
   {
      FT245RBitbangController* ftDevice = new FT245RBitbangController();
      ftDevice->SetName(StringExtensions::Build("FT245RBitbangController {0}", std::to_string(deviceIndex)));
      ftDevice->SetSerialNumber(deviceList[deviceIndex].serial);
      ftDevice->SetDescription(deviceList[deviceIndex].desc);
      ftDevice->SetId(deviceIndex);

      Log::Write(StringExtensions::Build("FT245RBitbangControllerAutoConfigurator.AutoConfig.. Detected FT245RBitbangController[{0}], name={1}, description: {2}, serial #{3}",
         std::to_string(deviceIndex), ftDevice->GetName(), ftDevice->GetDescription(), ftDevice->GetSerialNumber()));

      if (!cabinet->GetOutputControllers()->Contains(ftDevice->GetName()))
      {
         cabinet->GetOutputControllers()->push_back(ftDevice);

         Log::Write(StringExtensions::Build("Detected and added FT245RBitbangController Id {0} with name {1}", std::to_string(deviceIndex), ftDevice->GetName()));

         int ledWizNumber = deviceIndex + 40;
         bool foundExistingToy = false;

         for (auto* toy : *cabinet->GetToys())
         {
            LedWizEquivalent* lwe = dynamic_cast<LedWizEquivalent*>(toy);
            if (lwe && lwe->GetLedWizNumber() == ledWizNumber)
            {
               foundExistingToy = true;
               break;
            }
         }

         if (!foundExistingToy)
         {
            LedWizEquivalent* lwe = new LedWizEquivalent();
            lwe->SetLedWizNumber(ledWizNumber);
            lwe->SetName(StringExtensions::Build("{0} Equivalent 1", ftDevice->GetName()));

            for (int outputIndex = 1; outputIndex <= 8; outputIndex++)
            {
               LedWizEquivalentOutput* lweo = new LedWizEquivalentOutput();
               lweo->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", ftDevice->GetName(), std::to_string(outputIndex)));
               lweo->SetLedWizEquivalentOutputNumber(outputIndex);
               lwe->GetOutputs().AddOutput(lweo);
            }

            if (!cabinet->GetToys()->Contains(lwe->GetName()))
            {
               cabinet->GetToys()->AddToy(lwe);
               Log::Write(StringExtensions::Build("Added LedwizEquivalent Nr. {0} with name {1} for FT245RBitbangController with Id {2}", std::to_string(lwe->GetLedWizNumber()),
                  lwe->GetName(), std::to_string(deviceIndex)));
            }
         }
      }
   }
}

}