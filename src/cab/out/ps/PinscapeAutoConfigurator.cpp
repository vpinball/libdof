#include "PinscapeAutoConfigurator.h"

#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../Cabinet.h"
#include "../../toys/ToyList.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../OutputControllerList.h"
#include "Pinscape.h"

namespace DOF
{

void PinscapeAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   Log::Write("Pinscape auto-configuration starting");

   const int unitBias = 50;

   std::vector<int> preconfigured;
   for (IOutputController* controller : *cabinet->GetOutputControllers())
   {
      Pinscape* ps = dynamic_cast<Pinscape*>(controller);
      if (ps)
         preconfigured.push_back(ps->GetNumber());
   }

   Pinscape::FindDevices();
   auto allDevices = Pinscape::GetAllDevices();

   std::vector<int> numbers;
   for (auto device : allDevices)
   {
      auto d = static_cast<Pinscape::Device*>(device);
      numbers.push_back(d->GetUnitNo());
   }

   for (int n : numbers)
   {
      if (std::find(preconfigured.begin(), preconfigured.end(), n) == preconfigured.end())
      {
         Pinscape* p = new Pinscape(n);
         if (!cabinet->GetOutputControllers()->Contains(p->GetName()))
         {
            cabinet->GetOutputControllers()->push_back(p);
            Log::Write(StringExtensions::Build("Detected and added Pinscape Controller (KL25Z) #{0} with name {1}", std::to_string(p->GetNumber()), p->GetName()));

            bool toyExists = false;
            for (IToy* toy : *cabinet->GetToys())
            {
               LedWizEquivalent* l = dynamic_cast<LedWizEquivalent*>(toy);
               if (l && l->GetLedWizNumber() == p->GetNumber() + unitBias)
               {
                  toyExists = true;
                  break;
               }
            }

            if (!toyExists)
            {
               LedWizEquivalent* lwe = new LedWizEquivalent();
               lwe->SetLedWizNumber(p->GetNumber() + unitBias);
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
                  Log::Write(StringExtensions::Build("Added LedWizEquivalent #{0} with name {1} for Pinscape Controller (KL25Z) #{2}, {3} output ports",
                     std::to_string(lwe->GetLedWizNumber()), lwe->GetName(), std::to_string(p->GetNumber()), std::to_string(p->GetNumberOfOutputs())));
               }
            }
         }
      }
   }
}

}
