#include "LedWizAutoConfigurator.h"

#include "../../../Log.h"
#include "../../Cabinet.h"
#include "../../toys/ToyList.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../OutputControllerList.h"
#include "LedWiz.h"
#include "../../../general/StringExtensions.h"

namespace DOF
{

LedWizAutoConfigurator::LedWizAutoConfigurator() { }

LedWizAutoConfigurator::~LedWizAutoConfigurator() { }

void LedWizAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   std::vector<int> preconfigured;
   for (IOutputController* oc : *cabinet->GetOutputControllers())
   {
      LedWiz* lw = dynamic_cast<LedWiz*>(oc);
      if (lw)
         preconfigured.push_back(lw->GetNumber());
   }

   std::vector<int> numbers = LedWiz::GetLedwizNumbers();
   for (int n : numbers)
   {
      if (std::find(preconfigured.begin(), preconfigured.end(), n) == preconfigured.end())
      {
         LedWiz* lw = new LedWiz(n);
         if (!cabinet->GetOutputControllers()->Contains(lw->GetName()))
         {
            cabinet->GetOutputControllers()->push_back(lw);
            Log::Write(StringExtensions::Build("Detected and added LedWiz Nr. {0} with name {1}", std::to_string(lw->GetNumber()), lw->GetName()));

            bool toyExists = false;
            for (IToy* t : *cabinet->GetToys())
            {
               LedWizEquivalent* lwe = dynamic_cast<LedWizEquivalent*>(t);
               if (lwe && lwe->GetLedWizNumber() == lw->GetNumber())
               {
                  toyExists = true;
                  break;
               }
            }

            if (!toyExists)
            {
               LedWizEquivalent* lwe = new LedWizEquivalent();
               lwe->SetLedWizNumber(lw->GetNumber());
               lwe->SetName(StringExtensions::Build("{0} Equivalent", lw->GetName()));

               for (int i = 1; i < 33; i++)
               {
                  auto lweo = new LedWizEquivalentOutput();
                  lweo->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", lw->GetName(), std::to_string(i)));
                  lweo->SetLedWizEquivalentOutputNumber(i);
                  lwe->GetOutputs().AddOutput(lweo);
               }

               if (!cabinet->GetToys()->Contains(lwe->GetName()))
               {
                  cabinet->GetToys()->push_back(lwe);
                  Log::Write(StringExtensions::Build(
                     "Added LedwizEquivalent Nr. {0} with name {1} for Ledwiz Nr. {2}", std::to_string(lwe->GetLedWizNumber()), lwe->GetName(), std::to_string(lw->GetNumber())));
               }
            }
         }
      }
   }
}

}