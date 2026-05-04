#include "PinOneAutoConfigurator.h"
#include "PinOne.h"
#include "PinOneCommunication.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../../../general/StringExtensions.h"
#include "../../../Log.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../toys/ToyList.h"

#include <libserialport.h>
#include <algorithm>
#include <thread>
#include <chrono>

namespace DOF
{

void PinOneAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
   Log::Write("PinOne auto-configuration starting");

   const int UnitBias = 10;

   std::vector<std::string> preconfigured;
   for (auto* controller : *cabinet->GetOutputControllers())
   {
      PinOne* pinOne = dynamic_cast<PinOne*>(controller);
      if (pinOne)
         preconfigured.push_back(pinOne->GetComPort());
   }

   std::string comPort = GetDevice();

   if (std::find(preconfigured.begin(), preconfigured.end(), comPort) == preconfigured.end() && !comPort.empty())
   {
      PinOne* p = new PinOne(comPort);
      if (!cabinet->GetOutputControllers()->Contains(p->GetName()))
      {
         cabinet->GetOutputControllers()->push_back(p);
         Log::Write(StringExtensions::Build("Detected and added PinOne Controller Nr. {0} with name {1}", std::to_string(p->GetNumber()), p->GetName()));

         bool foundEquivalent = false;
         for (auto* toy : *cabinet->GetToys())
         {
            LedWizEquivalent* lwe = dynamic_cast<LedWizEquivalent*>(toy);
            if (lwe && lwe->GetLedWizNumber() == p->GetNumber() + UnitBias)
            {
               foundEquivalent = true;
               break;
            }
         }

         if (!foundEquivalent)
         {
            LedWizEquivalent* lwe = new LedWizEquivalent();
            lwe->SetLedWizNumber(p->GetNumber() + UnitBias);
            lwe->SetName(StringExtensions::Build("{0} Equivalent", p->GetName()));

            for (int i = 1; i <= p->GetNumberOfOutputs(); i++)
            {
               LedWizEquivalentOutput* lweo = new LedWizEquivalentOutput();
               lweo->SetOutputName(StringExtensions::Build("{0}\\{0}.{1:00}", p->GetName(), std::to_string(i)));
               lweo->SetLedWizEquivalentOutputNumber(i);
               lwe->GetOutputs().AddOutput(lweo);
            }

            if (!cabinet->GetToys()->Contains(lwe->GetName()))
            {
               cabinet->GetToys()->push_back(lwe);
               Log::Write(StringExtensions::Build("Added LedwizEquivalent Nr. {0} with name {1} for PinOne Controller Nr. {2}, {3}", std::to_string(lwe->GetLedWizNumber()), lwe->GetName(),
                  std::to_string(p->GetNumber()), std::to_string(p->GetNumberOfOutputs())));
            }
         }
      }
   }
}

std::string PinOneAutoConfigurator::TestSerialPort(const char* portName)
{
   struct sp_port* port = nullptr;

   try
   {
      if (sp_get_port_by_name(portName, &port) != SP_OK)
         return "";

      if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK)
      {
         sp_free_port(port);
         return "";
      }

      sp_set_baudrate(port, 2000000);
      sp_set_bits(port, 8);
      sp_set_parity(port, SP_PARITY_NONE);
      sp_set_stopbits(port, 1);
      sp_set_dtr(port, SP_DTR_ON);
      sp_set_rts(port, SP_RTS_OFF);
      sp_set_cts(port, SP_CTS_IGNORE);
      sp_set_dsr(port, SP_DSR_IGNORE);
      sp_set_xon_xoff(port, SP_XONXOFF_DISABLED);

      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      sp_flush(port, SP_BUF_BOTH);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      uint8_t command[] = { 0, 251, 0, 0, 0, 0, 0, 0, 0 };
      sp_blocking_write(port, command, 9, 100);

      char buffer[256];
      int bytesRead = sp_blocking_read(port, buffer, sizeof(buffer) - 1, 100);

      if (bytesRead > 0)
      {
         buffer[bytesRead] = '\0';
         std::string result(buffer);

         while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
            result.pop_back();

         if (result == "DEBUG,CSD Board Connected")
         {
            sp_close(port);
            sp_free_port(port);
            return std::string(portName);
         }
      }

      sp_flush(port, SP_BUF_BOTH);
      sp_set_rts(port, SP_RTS_OFF);
      sp_set_dtr(port, SP_DTR_OFF);

      sp_close(port);
      sp_free_port(port);
   }
   catch (...)
   {
      if (port)
      {
         try
         {
            sp_flush(port, SP_BUF_BOTH);
            sp_set_rts(port, SP_RTS_OFF);
            sp_set_dtr(port, SP_DTR_OFF);
         }
         catch (...)
         {
         }

         sp_close(port);
         sp_free_port(port);
      }
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(10));

   return "";
}

std::string PinOneAutoConfigurator::GetDevice()
{
   struct sp_port** portList;
   if (sp_list_ports(&portList) != SP_OK)
      return "";

   for (int i = 0; portList[i] != nullptr; i++)
   {
      char* portName = sp_get_port_name(portList[i]);
      if (!portName)
         continue;

      std::string result = TestSerialPort(portName);
      if (!result.empty())
      {
         sp_free_port_list(portList);
         return result;
      }
   }

   sp_free_port_list(portList);

   std::string comPort = "";
   PinOneCommunication communication("");
   if (communication.ConnectToServer())
      comPort = communication.GetCOMPort();

   return comPort;
}

}