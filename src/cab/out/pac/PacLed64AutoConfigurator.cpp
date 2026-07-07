#include "PacLed64AutoConfigurator.h"
#include "PacLed64.h"
#include "PacDriveSingleton.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../../toys/ToyList.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <algorithm>

namespace DOF
{

void PacLed64AutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   if (!cabinet)
      return;

   Log::Write("PacLed64 auto-configuration starting");

   std::vector<int> pacLed64Ids = PacDriveSingleton::GetInstance().PacLed64GetIdList();

   for (int id : pacLed64Ids)
   {
      bool found = false;
      for (IOutputController* oc : *cabinet->GetOutputControllers())
      {
         PacLed64* pacLed64 = dynamic_cast<PacLed64*>(oc);
         if (pacLed64 && pacLed64->GetId() == id)
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         PacLed64* pacLed = new PacLed64();
         pacLed->SetId(id);

         if (!cabinet->GetOutputControllers()->Contains(pacLed->GetName()))
         {
            cabinet->GetOutputControllers()->push_back(pacLed);

            Log::Write(StringExtensions::Build("Detected and added PacLed64 Id {0} with name {1}", std::to_string(pacLed->GetId()), pacLed->GetName()));

            int ledWizNumber = pacLed->GetId() - 1 + 20;

            bool toyFound = false;
            for (IToy* toy : *cabinet->GetToys())
            {
               LedWizEquivalent* lwe = dynamic_cast<LedWizEquivalent*>(toy);
               if (lwe && lwe->GetLedWizNumber() == ledWizNumber)
               {
                  toyFound = true;
                  break;
               }
            }

            if (!toyFound)
            {
               LedWizEquivalent* lwe = new LedWizEquivalent();
               lwe->SetLedWizNumber(ledWizNumber);
               lwe->SetName(StringExtensions::Build("{0} Equivalent 1", pacLed->GetName()));

               for (int i = 1; i <= 64; i++)
               {
                  LedWizEquivalentOutput* lweo = new LedWizEquivalentOutput();
                  lweo->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", pacLed->GetName(), std::to_string(i)));
                  lweo->SetLedWizEquivalentOutputNumber(i);
                  lwe->GetOutputs().AddOutput(lweo);
               }

               if (!cabinet->GetToys()->Contains(lwe->GetName()))
               {
                  cabinet->GetToys()->AddToy(lwe);
                  Log::Write(StringExtensions::Build(
                     "Added LedwizEquivalent Nr. {0} with name {1} for PacLed64 with Id {2}", std::to_string(lwe->GetLedWizNumber()), lwe->GetName(), std::to_string(pacLed->GetId())));
               }
            }
         }
      }
   }
}

}