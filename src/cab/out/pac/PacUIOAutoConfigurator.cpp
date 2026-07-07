#include "PacUIOAutoConfigurator.h"
#include "PacUIO.h"
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

void PacUIOAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   if (!cabinet)
      return;

   Log::Write("PacUIO auto-configuration starting");

   std::vector<int> pacUIOIds = PacDriveSingleton::GetInstance().PacUIOGetIdList();

   for (int id : pacUIOIds)
   {
      bool found = false;
      for (IOutputController* oc : *cabinet->GetOutputControllers())
      {
         PacUIO* pacUIO = dynamic_cast<PacUIO*>(oc);
         if (pacUIO && pacUIO->GetId() == id)
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         PacUIO* pacUIO = new PacUIO();
         pacUIO->SetId(id);

         Log::Write(StringExtensions::Build("PacUIOAutoConfigurator.AutoConfig.. Detected PacUIO[{0}], name={1}", std::to_string(id), pacUIO->GetName()));

         if (!cabinet->GetOutputControllers()->Contains(pacUIO->GetName()))
         {
            cabinet->GetOutputControllers()->push_back(pacUIO);

            Log::Write(StringExtensions::Build("Detected and added PacUIO Id {0} with name {1}", std::to_string(pacUIO->GetId()), pacUIO->GetName()));

            bool toyFound = false;
            int ledWizNumber = pacUIO->GetId() + 27;
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
               lwe->SetName(StringExtensions::Build("{0} Equivalent 1", pacUIO->GetName()));

               for (int i = 1; i <= 96; i++)
               {
                  LedWizEquivalentOutput* lweo = new LedWizEquivalentOutput();
                  lweo->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", pacUIO->GetName(), std::to_string(i)));
                  lweo->SetLedWizEquivalentOutputNumber(i);
                  lwe->GetOutputs().AddOutput(lweo);
               }

               if (!cabinet->GetToys()->Contains(lwe->GetName()))
               {
                  cabinet->GetToys()->AddToy(lwe);
                  Log::Write(StringExtensions::Build(
                     "Added LedwizEquivalent Nr. {0} with name {1} for PacUIO with Id {2}", std::to_string(lwe->GetLedWizNumber()), lwe->GetName(), std::to_string(pacUIO->GetId())));
               }
            }
         }
      }
   }
}

}