#include "PinOneAutoConfigurator.h"
#include "PinOne.h"
#include "PinOneCommunication.h"
#include "PinOneHidCommunication.h"
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
#include <regex>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")
#endif

namespace DOF
{

const std::string PinOneAutoConfigurator::LegacyHardwareId = "VID_0E8F&PID_9207";

void PinOneAutoConfigurator::AutoConfig(Cabinet* cabinet)
{
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

std::string PinOneAutoConfigurator::GetDevice()
{
   PinOneHidDevice* hidDevice = PinOneHidCommunication::FindDevice();
   if (hidDevice != nullptr)
   {
      delete hidDevice;
      return PinOne::HidSentinel;
   }

   bool wmiAvailable;
   std::string legacyPort = FindLegacyComPort(wmiAvailable);
   if (legacyPort != "")
   {
      if (TryConnect(legacyPort))
         return legacyPort;
   }
   else if (!wmiAvailable)
   {
      struct sp_port** portList;
      if (sp_list_ports(&portList) == SP_OK)
      {
         std::string found = "";
         for (int i = 0; portList[i] != nullptr; i++)
         {
            const char* portName = sp_get_port_name(portList[i]);
            if (portName && TryConnect(portName))
            {
               found = portName;
               break;
            }
         }
         sp_free_port_list(portList);
         if (found != "")
            return found;
      }
   }

   std::string comPort = "";
   PinOneCommunication communication("");
   if (communication.ConnectToServer())
      comPort = communication.GetCOMPort();

   return comPort;
}

std::string PinOneAutoConfigurator::FindLegacyComPort(bool& wmiAvailable)
{
   wmiAvailable = true;

#ifdef _WIN32
   HDEVINFO devInfo = SetupDiGetClassDevsA(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
   if (devInfo == INVALID_HANDLE_VALUE)
   {
      Log::Exception("Error searching for legacy PinOne COM port");
      wmiAvailable = false;
      return "";
   }

   std::string result = "";
   SP_DEVINFO_DATA devInfoData;
   devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
   for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); i++)
   {
      char instanceId[512];
      if (!SetupDiGetDeviceInstanceIdA(devInfo, &devInfoData, instanceId, sizeof(instanceId), nullptr))
         continue;

      if (StringExtensions::ToUpper(instanceId).find(LegacyHardwareId) == std::string::npos)
         continue;

      char friendlyName[512];
      if (!SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData, SPDRP_FRIENDLYNAME, nullptr, reinterpret_cast<PBYTE>(friendlyName), sizeof(friendlyName), nullptr))
         continue;

      std::string name(friendlyName);
      if (name.empty())
         continue;

      std::smatch match;
      if (std::regex_search(name, match, std::regex("\\(COM\\d+\\)")))
      {
         result = match.str().substr(1, match.str().length() - 2);
         break;
      }
   }

   SetupDiDestroyDeviceInfoList(devInfo);

   return result;
#else
   struct sp_port** portList;
   if (sp_list_ports(&portList) != SP_OK)
   {
      Log::Exception("Error searching for legacy PinOne COM port");
      wmiAvailable = false;
      return "";
   }

   std::string result = "";
   for (int i = 0; portList[i] != nullptr; i++)
   {
      if (sp_get_port_transport(portList[i]) != SP_TRANSPORT_USB)
         continue;

      int vid = 0;
      int pid = 0;
      if (sp_get_port_usb_vid_pid(portList[i], &vid, &pid) != SP_OK)
         continue;

      char hardwareId[32];
      snprintf(hardwareId, sizeof(hardwareId), "VID_%04X&PID_%04X", vid, pid);
      if (LegacyHardwareId != hardwareId)
         continue;

      const char* portName = sp_get_port_name(portList[i]);
      if (portName)
      {
         result = portName;
         break;
      }
   }

   sp_free_port_list(portList);

   return result;
#endif
}

bool PinOneAutoConfigurator::TryConnect(const std::string& sp)
{
   struct sp_port* port = nullptr;
   if (sp_get_port_by_name(sp.c_str(), &port) != SP_OK || !port)
      return false;

   if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK)
   {
      sp_free_port(port);
      return false;
   }

   struct sp_port_config* config = nullptr;
   if (sp_new_config(&config) == SP_OK)
   {
      sp_set_config_baudrate(config, 2000000);
      sp_set_config_bits(config, 8);
      sp_set_config_parity(config, SP_PARITY_NONE);
      sp_set_config_stopbits(config, 1);
      sp_set_config_rts(config, SP_RTS_OFF);
      sp_set_config_cts(config, SP_CTS_IGNORE);
      sp_set_config_dtr(config, SP_DTR_ON);
      sp_set_config_dsr(config, SP_DSR_IGNORE);
      sp_set_config_xon_xoff(config, SP_XONXOFF_DISABLED);
      sp_set_config(port, config);
      sp_free_config(config);
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(20));
   sp_flush(port, SP_BUF_BOTH);
   std::this_thread::sleep_for(std::chrono::milliseconds(100));

   bool connected = false;
   uint8_t command[] = { 0, 251, 0, 0, 0, 0, 0, 0, 0 };
   if (sp_blocking_write(port, command, sizeof(command), 100) == static_cast<int>(sizeof(command)))
   {
      std::string line;
      auto lineStart = std::chrono::steady_clock::now();
      while (true)
      {
         int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lineStart).count());
         if (elapsed >= 100)
            break;

         char c;
         if (sp_blocking_read(port, &c, 1, 100 - elapsed) <= 0)
            break;

         if (c != '\n')
         {
            line += c;
            continue;
         }

         if (!line.empty() && line.back() == '\r')
            line.pop_back();

         if (line == "DEBUG,CSD Board Connected")
         {
            connected = true;
            break;
         }

         line.clear();
         lineStart = std::chrono::steady_clock::now();
      }
   }

   if (!connected)
   {
      sp_flush(port, SP_BUF_BOTH);
      sp_set_rts(port, SP_RTS_OFF);
      sp_set_dtr(port, SP_DTR_OFF);
   }

   sp_close(port);
   sp_free_port(port);

   return connected;
}

}
