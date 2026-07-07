#include "DudesCabAutoConfigurator.h"
#include "DudesCab.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../../toys/ToyList.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"

namespace DOF
{

void DudesCabAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   Log::Write("DudesCab auto-configuration starting");

   std::vector<DudesCab::Device*> devices = DudesCab::AllDevices();

   for (DudesCab::Device* device : devices)
   {
      if (device->UnitNo() >= 1 && device->UnitNo() <= 5)
      {
         std::string controllerName = StringExtensions::Build("DudesCab Controller {0:00}", std::to_string(device->UnitNo()));

         if (!cabinet->GetOutputControllers()->Contains(controllerName))
         {
            DudesCab* dudesCab = new DudesCab(device->UnitNo());
            dudesCab->SetName(controllerName);

            cabinet->GetOutputControllers()->push_back(dudesCab);

            Log::Write(StringExtensions::Build("Detected and added DudesCab Controller {0} with {1} outputs", controllerName, std::to_string(device->NumOutputs())));

            int ledWizNumber = 90 + device->UnitNo() - 1;

            bool foundExistingToy = false;
            for (auto* toy : *cabinet->GetToys())
            {
               if (auto* lwe = dynamic_cast<LedWizEquivalent*>(toy))
               {
                  if (lwe->GetLedWizNumber() == ledWizNumber)
                  {
                     foundExistingToy = true;
                     break;
                  }
               }
            }

            if (!foundExistingToy)
            {
               LedWizEquivalent* lwe = new LedWizEquivalent();
               lwe->SetLedWizNumber(ledWizNumber);
               lwe->SetName(StringExtensions::Build("{0} Equivalent 1", controllerName));

               for (int outputIndex = 1; outputIndex <= device->NumOutputs(); outputIndex++)
               {
                  LedWizEquivalentOutput* lweo = new LedWizEquivalentOutput();
                  lweo->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", controllerName, std::to_string(outputIndex)));
                  lweo->SetLedWizEquivalentOutputNumber(outputIndex);
                  lwe->GetOutputs().AddOutput(lweo);
               }

               if (!cabinet->GetToys()->Contains(lwe->GetName()))
               {
                  cabinet->GetToys()->AddToy(lwe);
                  Log::Write(StringExtensions::Build("Added LedwizEquivalent Nr. {0} with name {1} for DudesCab Controller with Unit {2}", std::to_string(lwe->GetLedWizNumber()),
                     lwe->GetName(), std::to_string(device->UnitNo())));
               }
            }
         }
      }
   }
}

}