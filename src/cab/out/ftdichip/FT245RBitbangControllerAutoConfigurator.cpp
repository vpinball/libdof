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

   for (uint32_t i = 0; i < amountDevices; i++)
   {
      FTDI* connectFTDI = new FTDI();

      FTDI::FT_DEVICE_INFO_NODE devInfo;
      uint32_t numDevs = 1;
      status = connectFTDI->GetDeviceInfoList(&devInfo, numDevs);

      if (status == FTDI::FT_OK && numDevs > 0)
      {
         deviceList.emplace_back(devInfo.SerialNumber, devInfo.Description);
      }
      delete connectFTDI;
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