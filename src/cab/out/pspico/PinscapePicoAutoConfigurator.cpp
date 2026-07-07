#include "PinscapePicoAutoConfigurator.h"
#include "PinscapePico.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../IOutputController.h"
#include "../../toys/ToyList.h"
#include "../../toys/IToy.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <algorithm>

namespace DOF
{

void PinscapePicoAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   if (!cabinet)
      return;

   const int unitBias = 119;

   Log::Write("PinscapePico auto-configuration starting");

   PinscapePico::FindDevices();
   auto allDevices = PinscapePico::GetAllDevices();

   if (allDevices.empty())
   {
      return;
   }

   Log::Write(StringExtensions::Build("Found {0} PinscapePico device(s), configuring...", std::to_string(allDevices.size())));

   std::vector<int> preconfigured;
   for (IOutputController* controller : *cabinet->GetOutputControllers())
   {
      PinscapePico* psp = dynamic_cast<PinscapePico*>(controller);
      if (psp)
         preconfigured.push_back(psp->GetNumber());
   }

   for (auto device : allDevices)
   {
      auto d = static_cast<PinscapePico::Device*>(device);

      int n = d->UnitNo();
      int dofUnitNumber = n + unitBias;

      Log::Write(StringExtensions::Build("Configuring PinscapePico unit {0} as DOF unit {1}", std::to_string(n), std::to_string(dofUnitNumber)));

      if (std::find(preconfigured.begin(), preconfigured.end(), n) == preconfigured.end())
      {
         auto p = new PinscapePico(n);

         if (!cabinet->GetOutputControllers()->Contains(p->GetName()))
         {
            cabinet->GetOutputControllers()->push_back(p);
            Log::Write(StringExtensions::Build("Detected and added PinscapePico Controller Nr. {0} with name {1}", std::to_string(n), p->GetName()));

            bool toyExists = false;
            for (IToy* toy : *cabinet->GetToys())
            {
               LedWizEquivalent* l = dynamic_cast<LedWizEquivalent*>(toy);
               if (l && l->GetLedWizNumber() == dofUnitNumber)
               {
                  toyExists = true;
                  break;
               }
            }

            if (!toyExists)
            {
               LedWizEquivalent* lwe = new LedWizEquivalent();
               lwe->SetLedWizNumber(dofUnitNumber);
               lwe->SetName(StringExtensions::Build("{0} Equivalent", p->GetName()));

               for (int i = 1; i <= p->GetNumberOfOutputs(); i++)
               {
                  auto output = new LedWizEquivalentOutput();
                  output->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", p->GetName(), std::to_string(i)));
                  output->SetLedWizEquivalentOutputNumber(i);
                  lwe->GetOutputs().AddOutput(output);
               }

               if (!cabinet->GetToys()->Contains(lwe->GetName()))
               {
                  cabinet->GetToys()->push_back(lwe);
                  Log::Write(StringExtensions::Build("Added LedWizEquivalent #{0} with name {1} for Pinscape Pico #{2}, {3} output ports", std::to_string(lwe->GetLedWizNumber()),
                     lwe->GetName(), std::to_string(p->GetNumber()), std::to_string(p->GetNumberOfOutputs())));
               }
            }
         }
      }
   }

   Log::Write("PinscapePico auto-configuration completed");
}

}