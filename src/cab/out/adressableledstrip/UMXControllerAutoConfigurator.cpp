#include "UMXControllerAutoConfigurator.h"
#include "UMXController.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <algorithm>

namespace DOF
{

std::vector<UMXDevice*> UMXControllerAutoConfigurator::s_devices;
bool UMXControllerAutoConfigurator::s_inited = false;
Cabinet* UMXControllerAutoConfigurator::s_cabinet = nullptr;

void UMXControllerAutoConfigurator::AddUMXDevice(UMXDevice* dev)
{
   for (auto* existing : s_devices)
   {
      if (existing == dev)
         return;
   }

   dev->m_unitNo = static_cast<int16_t>(s_devices.size() + 1);
   s_devices.push_back(dev);

   try
   {
      if (s_inited && s_cabinet != nullptr)
      {
         UMXController* umxC = new UMXController();
         umxC->SetNumber(dev->UnitNo());
         Log::Instrumentation(
            "UMX", StringExtensions::Build("Adding new device {0} & controller {1} to cabinet after UMXControllerAutoConfigurator initialization", dev->m_name, umxC->GetName()));
         umxC->UpdateCabinetFromConfig(s_cabinet);
      }
   }
   catch (const std::exception& e)
   {
      Log::Exception(e.what());
   }
}

void UMXControllerAutoConfigurator::RemoveAllUMXDevices()
{
   for (auto* dev : s_devices)
      delete dev;
   s_devices.clear();
}

std::vector<UMXDevice*> UMXControllerAutoConfigurator::AllDevices() { return s_devices; }

void UMXControllerAutoConfigurator::ClearDevices()
{
   for (auto* dev : s_devices)
      delete dev;
   s_devices.clear();
   s_inited = false;
   s_cabinet = nullptr;
}

void UMXControllerAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   s_cabinet = cabinet;

   std::vector<int> preconfigured;
   for (auto* oc : *cabinet->GetOutputControllers())
   {
      UMXController* umxC = dynamic_cast<UMXController*>(oc);
      if (umxC)
         preconfigured.push_back(umxC->GetNumber());
   }

   for (auto deviceIt = s_devices.begin(); deviceIt != s_devices.end();)
   {
      UMXDevice* device = *deviceIt;
      bool found = false;
      for (int num : preconfigured)
      {
         if (num == device->UnitNo())
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         UMXController* umxC = new UMXController();
         try
         {
            umxC->SetNumber(device->UnitNo());
            umxC->UpdateCabinetFromConfig(cabinet);
         }
         catch (const std::exception& e)
         {
            Log::Exception(StringExtensions::Build("UMX device {0} could not be configured and is skipped: {1}", device->m_name, e.what()));
            OutputControllerList* controllers = cabinet->GetOutputControllers();
            if (std::find(controllers->begin(), controllers->end(), umxC) == controllers->end())
            {
               delete umxC;
               device->Release();
               deviceIt = s_devices.erase(deviceIt);
               delete device;
               continue;
            }
         }
      }
      ++deviceIt;
   }

   s_inited = true;
}

}
