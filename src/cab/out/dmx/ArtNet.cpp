#include "ArtNet.h"
#include "artnetengine/Engine.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <stdexcept>

namespace DOF
{

ArtNet::ArtNet()
   : m_engine(nullptr)
   , m_universe(0)
   , m_broadcastAddress("")
{
}

ArtNet::~ArtNet() { }

int ArtNet::GetNumberOfConfiguredOutputs() { return DMX_CHANNELS; }

bool ArtNet::VerifySettings() { return true; }

void ArtNet::UpdateOutputs(const std::vector<uint8_t>& outputValues)
{
   try
   {
      if (m_engine != nullptr)
      {
         if (outputValues.size() == DMX_CHANNELS)
         {
            if (!m_engine->SendDMX(m_broadcastAddress, m_universe, outputValues, DMX_CHANNELS))
               throw std::runtime_error("the Art-Net packet was not sent");
         }
         else
         {
            std::string msg = StringExtensions::Build("{0} {1} sent the wrong number of bytes to output.", GetXmlElementName(), GetName());
            Log::Exception(msg);
            throw std::runtime_error(msg);
         }
      }
      else
      {
         std::string msg = StringExtensions::Build(
            "{0} {1} (Universe: {2}, Broadcast Address: {3}) is not connected.", { GetXmlElementName(), GetName(), std::to_string(m_universe), m_broadcastAddress });
         Log::Exception(msg);
         throw std::runtime_error(msg);
      }
   }
   catch (const std::exception& e)
   {
      std::string msg = StringExtensions::Build(
         "{0} {1} (Universe: {2}, Broadcast Address: {3}) could not send data: {4}", { GetXmlElementName(), GetName(), std::to_string(m_universe), m_broadcastAddress, e.what() });
      Log::Exception(msg);
      throw std::runtime_error(msg);
   }
}

void ArtNet::ConnectToController()
{
   if (m_engine == nullptr)
   {
      try
      {
         m_engine = Engine::GetInstance();
         if (!m_engine->Reopen())
            throw std::runtime_error("the Art-Net socket could not be opened");
         std::vector<uint8_t> zeroData(DMX_CHANNELS, 0);
         m_engine->SendDMX(m_broadcastAddress, m_universe, zeroData, DMX_CHANNELS);
      }
      catch (const std::exception& e)
      {
         m_engine = nullptr;
         std::string msg = StringExtensions::Build(
            "{0} {1} (Universe: {2}, Broadcast Address: {3}) could not connect: {4}", { GetXmlElementName(), GetName(), std::to_string(m_universe), m_broadcastAddress, e.what() });
         Log::Exception(msg);
         throw std::runtime_error(msg);
      }
   }
}

void ArtNet::DisconnectFromController()
{
   bool sent = true;
   if (m_engine != nullptr)
   {
      try
      {
         std::vector<uint8_t> zeroData(DMX_CHANNELS, 0);
         sent = m_engine->SendDMX(m_broadcastAddress, m_universe, zeroData, DMX_CHANNELS);
      }
      catch (...)
      {
         sent = false;
      }
      m_engine = nullptr;
      m_disconnectTurnedOff = sent;
   }
   if (!sent)
      throw std::runtime_error("the Art-Net packet that turns the outputs off was not sent");
}

bool ArtNet::FromXml(const tinyxml2::XMLElement* element)
{
   if (!OutputControllerCompleteBase::FromXml(element))
      return false;

   const tinyxml2::XMLElement* universeElement = element->FirstChildElement("Universe");
   if (universeElement && universeElement->GetText())
   {
      try
      {
         int universe = std::stoi(universeElement->GetText());
         SetUniverse(static_cast<short>(universe));
      }
      catch (...)
      {
         return false;
      }
   }

   const tinyxml2::XMLElement* broadcastElement = element->FirstChildElement("BroadcastAddress");
   if (broadcastElement && broadcastElement->GetText())
   {
      SetBroadcastAddress(broadcastElement->GetText());
   }

   return true;
}

tinyxml2::XMLElement* ArtNet::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerCompleteBase::ToXml(doc);

   tinyxml2::XMLElement* universeElement = doc.NewElement("Universe");
   universeElement->SetText(m_universe);
   element->InsertEndChild(universeElement);

   if (!m_broadcastAddress.empty())
   {
      tinyxml2::XMLElement* broadcastElement = doc.NewElement("BroadcastAddress");
      broadcastElement->SetText(m_broadcastAddress.c_str());
      element->InsertEndChild(broadcastElement);
   }

   return element;
}

}