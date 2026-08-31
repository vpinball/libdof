#include "PinControl.h"
#include "../../Cabinet.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../../general/MathExtensions.h"
#include <cstring>

namespace DOF
{

PinControl::PinControl()
   : m_oldValues(nullptr)
   , m_port(nullptr)
{
}

PinControl::~PinControl() { }

int PinControl::GetNumberOfConfiguredOutputs() { return 7 + 3; }

bool PinControl::VerifySettings()
{
   if (StringExtensions::IsNullOrWhiteSpace(m_comPort))
   {
      Log::Warning(StringExtensions::Build("ComPort is not set for {0} {1}.", "PinControl", GetName()));
      return false;
   }

   struct sp_port** ports;
   if (sp_list_ports(&ports) == SP_OK)
   {
      bool found = false;
      for (int i = 0; ports[i]; i++)
      {
         const char* portName = sp_get_port_name(ports[i]);
         if (portName && StringExtensions::ToLower(portName) == StringExtensions::ToLower(m_comPort))
         {
            found = true;
            break;
         }
      }
      sp_free_port_list(ports);

      if (!found)
      {
         struct sp_port* port = nullptr;
         if (sp_get_port_by_name(m_comPort.c_str(), &port) == SP_OK && port)
         {
            sp_free_port(port);
            found = true;
         }
      }

      if (!found)
      {
         Log::Warning(StringExtensions::Build("ComPort {0} is defined for {1} {2}, but does not exist.", m_comPort, "PinControl", GetName()));
         return false;
      }
   }

   return true;
}

void PinControl::UpdateOutputs(const std::vector<uint8_t>& outputValues)
{
   if (m_port)
   {
      std::lock_guard<std::recursive_mutex> lock(m_portLocker);

      for (int i = 0; i < 7 && i < static_cast<int>(outputValues.size()); i++)
      {
         if (m_oldValues == nullptr || m_oldValues[i] != outputValues[i])
         {
            std::string command;
            if (i == 0 && outputValues[i] != 0)
               command = StringExtensions::Build("{0},{1},0,0,{2}#", std::to_string(i + 1), outputValues[i] == 0 ? "2" : "1", std::to_string(outputValues[i]));
            else
               command = StringExtensions::Build("{0},{1}#", std::to_string(i + 1), outputValues[i] == 0 ? "2" : "1");

            sp_blocking_write(m_port, command.c_str(), command.length(), 1000);
         }
      }

      bool colorChanged = false;
      bool isBlack = true;
      for (int i = 7; i < 10 && i < static_cast<int>(outputValues.size()); i++)
      {
         if (m_oldValues == nullptr || m_oldValues[i] != outputValues[i])
            colorChanged = true;
         if (outputValues[i] != 0)
            isBlack = false;
      }

      if (colorChanged)
      {
         std::string command;
         if (isBlack)
            command = "9,2#";
         else
            command = StringExtensions::Build("9,1,{0},{1},{2}#", outputValues.size() > 7 ? std::to_string(outputValues[7]) : "0",
               outputValues.size() > 8 ? std::to_string(outputValues[8]) : "0", outputValues.size() > 9 ? std::to_string(outputValues[9]) : "0");

         sp_blocking_write(m_port, command.c_str(), command.length(), 1000);
      }

      if (m_oldValues == nullptr)
         m_oldValues = new uint8_t[GetNumberOfConfiguredOutputs()];

      size_t copySize = std::min(static_cast<size_t>(GetNumberOfConfiguredOutputs()), outputValues.size());
      std::memcpy(m_oldValues, outputValues.data(), copySize);
   }
   else
      throw std::runtime_error(StringExtensions::Build("COM port {0} is not initialized for {1} {2}.", m_comPort, "PinControl", GetName()));
}

void PinControl::ConnectToController()
{
   std::lock_guard<std::recursive_mutex> lock(m_portLocker);

   try
   {
      if (m_port != nullptr)
         DisconnectFromController();

      delete[] m_oldValues;
      m_oldValues = nullptr;

      if (sp_get_port_by_name(m_comPort.c_str(), &m_port) != SP_OK)
         throw std::runtime_error("Failed to get port by name");

      if (sp_open(m_port, SP_MODE_READ_WRITE) != SP_OK)
      {
         sp_free_port(m_port);
         m_port = nullptr;
         throw std::runtime_error("Failed to open port");
      }

      sp_set_baudrate(m_port, 115200);
      sp_set_bits(m_port, 8);
      sp_set_parity(m_port, SP_PARITY_NONE);
      sp_set_stopbits(m_port, 1);
      sp_set_flowcontrol(m_port, SP_FLOWCONTROL_NONE);
   }
   catch (const std::exception& e)
   {
      std::string msg = StringExtensions::Build("A exception occurred while opening comport {0} for {1} {2}.", m_comPort, "PinControl", GetName());
      Log::Exception(msg);
      throw std::runtime_error(msg);
   }
}

void PinControl::DisconnectFromController()
{
   std::lock_guard<std::recursive_mutex> lock(m_portLocker);

   if (m_port != nullptr)
   {
      sp_close(m_port);
      sp_free_port(m_port);
      m_port = nullptr;
      delete[] m_oldValues;
      m_oldValues = nullptr;
   }
}

bool PinControl::FromXml(const tinyxml2::XMLElement* element)
{
   if (!OutputControllerCompleteBase::FromXml(element))
      return false;

   const tinyxml2::XMLElement* comPortElement = element->FirstChildElement("ComPort");
   if (comPortElement && comPortElement->GetText())
   {
      SetComPort(comPortElement->GetText());
   }

   return true;
}

tinyxml2::XMLElement* PinControl::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerCompleteBase::ToXml(doc);

   if (!m_comPort.empty())
   {
      tinyxml2::XMLElement* comPortElement = doc.NewElement("ComPort");
      comPortElement->SetText(m_comPort.c_str());
      element->InsertEndChild(comPortElement);
   }

   return element;
}

}