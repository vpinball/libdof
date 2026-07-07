#include "PinOne.h"
#include "PinOneCommunication.h"
#include "../../Cabinet.h"
#include "../../../general/StringExtensions.h"
#include "../../../general/MathExtensions.h"
#include "../../../Log.h"
#include <algorithm>
#include <thread>
#include <cmath>

namespace DOF
{

PinOne::PinOne()
{
   SetComPort("");
   SetNumber(1);
}

PinOne::PinOne(const std::string& comPort)
{
   SetComPort(comPort);
   SetNumber(1);
}

PinOne::~PinOne()
{
   if (m_pinOneCommunication)
   {
      delete m_pinOneCommunication;
      m_pinOneCommunication = nullptr;
   }
}

void PinOne::SetNumber(int value)
{
   if (!MathExtensions::IsBetween(value, 1, 16))
      throw std::runtime_error(StringExtensions::Build("PinOne Unit Numbers must be between 1-16. The supplied number {0} is out of range.", std::to_string(value)));

   std::lock_guard<std::mutex> lock(m_numberUpdateLocker);

   if (m_number != value)
   {
      if (GetName().empty() || GetName() == StringExtensions::Build("PinOne Controller {0:00}", std::to_string(m_number)))
      {
         SetName(StringExtensions::Build("PinOne Controller {0:00}", StringExtensions::FormatNumber(value, 2)));
      }

      m_number = value;

      SetNumberOfOutputs(63);
      m_oldOutputValues.assign(GetNumberOfOutputs(), 255);
   }
}

void PinOne::SetMinCommandIntervalMs(int value)
{
   m_minCommandIntervalMs = MathExtensions::Limit(value, 0, 1000);
   m_minCommandIntervalMsSet = true;
}

void PinOne::SetComPort(const std::string& value)
{
   m_comPort = value;
   m_comPortSet = true;
}

void PinOne::Init(Cabinet* cabinet)
{
   if (!m_minCommandIntervalMsSet && cabinet->GetOwner()->GetConfigurationSettings().find("PinOneDefaultMinCommandIntervalMs") != cabinet->GetOwner()->GetConfigurationSettings().end())
   {
      auto it = cabinet->GetOwner()->GetConfigurationSettings().find("PinOneDefaultMinCommandIntervalMs");
      if (it != cabinet->GetOwner()->GetConfigurationSettings().end())
      {
         try
         {
            SetMinCommandIntervalMs(std::stoi(it->second));
         }
         catch (...)
         {
         }
      }
   }

   if (!m_comPortSet && cabinet->GetOwner()->GetConfigurationSettings().find("PinOneComPort") != cabinet->GetOwner()->GetConfigurationSettings().end())
   {
      auto it = cabinet->GetOwner()->GetConfigurationSettings().find("PinOneComPort");
      if (it != cabinet->GetOwner()->GetConfigurationSettings().end())
         SetComPort(it->second);
   }

   OutputControllerFlexCompleteBase::Init(cabinet);
}

void PinOne::Finish() { OutputControllerFlexCompleteBase::Finish(); }

bool PinOne::VerifySettings()
{
   if (StringExtensions::IsNullOrWhiteSpace(m_comPort))
   {
      Log::Warning(StringExtensions::Build("ComPort is not set for {0} {1}.", "PinOne", GetName()));
      return false;
   }

   return true;
}

void PinOne::UpdateOutputs(const std::vector<uint8_t>& newOutputValues)
{
   uint8_t pfx = 200;
   for (int i = 0; i < GetNumberOfOutputs(); i += 7, ++pfx)
   {
      int lim = std::min(i + 7, GetNumberOfOutputs());
      for (int j = i; j < lim; ++j)
      {
         if (newOutputValues[j] != m_oldOutputValues[j])
         {
            UpdateDelay();
            std::vector<uint8_t> buf(9);
            buf[0] = 0;
            buf[1] = pfx;
            std::copy(newOutputValues.begin() + i, newOutputValues.begin() + lim, buf.begin() + 2);

            if (m_pinOneCommunication)
               m_pinOneCommunication->Write(buf);

            std::copy(newOutputValues.begin() + i, newOutputValues.begin() + lim, m_oldOutputValues.begin() + i);
            break;
         }
      }
   }
}

void PinOne::UpdateDelay()
{
   auto now = std::chrono::steady_clock::now();
   auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdate).count();

   if (elapsed < m_minCommandIntervalMs)
   {
      int sleepMs = MathExtensions::Limit(static_cast<int>(m_minCommandIntervalMs - elapsed), 0, m_minCommandIntervalMs);
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
   }
   m_lastUpdate = std::chrono::steady_clock::now();
}

void PinOne::ConnectToController()
{
   try
   {
      std::lock_guard<std::recursive_mutex> lock(m_portLocker);

      if (m_pinOneCommunication)
         DisconnectFromController();

      m_pinOneCommunication = new PinOneCommunication(m_comPort);
      if (!m_pinOneCommunication->ConnectToServer())
      {
         if (m_pinOneCommunication->CreateServer())
         {
            if (!m_pinOneCommunication->ConnectToServer())
               throw std::runtime_error("Unable to connect to server after new creation");
         }
         else
         {
            throw std::runtime_error("Unable to create server");
         }
      }
   }
   catch (const std::exception& e)
   {
      std::string msg = StringExtensions::Build("A exception occurred while opening comport {2} for {0} {1}.", "PinOne", GetName(), m_comPort);
      Log::Exception(StringExtensions::Build("{0}: {1}", msg, e.what()));
      throw std::runtime_error(msg);
   }
}

void PinOne::DisconnectFromController()
{
   std::lock_guard<std::recursive_mutex> lock(m_portLocker);

   if (m_pinOneCommunication)
   {
      m_pinOneCommunication->DisconnectFromServer();
      delete m_pinOneCommunication;
      m_pinOneCommunication = nullptr;
   }
}

bool PinOne::FromXml(const tinyxml2::XMLElement* element)
{
   if (!OutputControllerFlexCompleteBase::FromXml(element))
      return false;

   const tinyxml2::XMLElement* numberElement = element->FirstChildElement("Number");
   if (numberElement && numberElement->GetText())
   {
      try
      {
         int number = std::stoi(numberElement->GetText());
         SetNumber(number);
      }
      catch (...)
      {
         return false;
      }
   }

   const tinyxml2::XMLElement* intervalElement = element->FirstChildElement("MinCommandIntervalMs");
   if (intervalElement && intervalElement->GetText())
   {
      try
      {
         int interval = std::stoi(intervalElement->GetText());
         SetMinCommandIntervalMs(interval);
      }
      catch (...)
      {
      }
   }

   const tinyxml2::XMLElement* comPortElement = element->FirstChildElement("ComPort");
   if (comPortElement && comPortElement->GetText())
   {
      SetComPort(comPortElement->GetText());
   }

   return true;
}

tinyxml2::XMLElement* PinOne::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerFlexCompleteBase::ToXml(doc);

   tinyxml2::XMLElement* numberElement = doc.NewElement("Number");
   numberElement->SetText(m_number);
   element->InsertEndChild(numberElement);

   tinyxml2::XMLElement* intervalElement = doc.NewElement("MinCommandIntervalMs");
   intervalElement->SetText(m_minCommandIntervalMs);
   element->InsertEndChild(intervalElement);

   if (!m_comPort.empty())
   {
      tinyxml2::XMLElement* comPortElement = doc.NewElement("ComPort");
      comPortElement->SetText(m_comPort.c_str());
      element->InsertEndChild(comPortElement);
   }

   return element;
}

}