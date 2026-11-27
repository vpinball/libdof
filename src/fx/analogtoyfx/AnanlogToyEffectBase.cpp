#include "AnanlogToyEffectBase.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../cab/Cabinet.h"
#include "../../cab/toys/ToyList.h"
#include "../../cab/toys/IAnalogAlphaToy.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"

namespace DOF
{

AnalogToyEffectBase::AnalogToyEffectBase()
   : m_table(nullptr)
   , m_toy(nullptr)
   , m_layer(nullptr)
   , m_layerNr(0)
{
}

void AnalogToyEffectBase::SetToyName(const std::string& value)
{
   if (m_toyName != value)
   {
      m_toyName = value;
      m_toy = nullptr;
      m_layer = nullptr;
   }
}

void AnalogToyEffectBase::ResolveName(Table* table) { }

void AnalogToyEffectBase::Init(Table* table)
{
   if (!StringExtensions::IsNullOrWhiteSpace(m_toyName) && table->GetPinball()->GetCabinet()->GetToys()->Contains(m_toyName))
   {
      IToy* toy = table->GetPinball()->GetCabinet()->GetToys()->FindByName(m_toyName);
      IAnalogAlphaToy* analogAlphaToy = dynamic_cast<IAnalogAlphaToy*>(toy);
      if (analogAlphaToy != nullptr)
      {
         m_toy = analogAlphaToy;
         m_layer = m_toy->GetLayers().GetOrCreateLayer(m_layerNr, 1);
      }
   }
   m_table = table;
   ResolveName(table);
}

void AnalogToyEffectBase::Finish()
{
   m_toy = nullptr;
   m_table = nullptr;
   EffectBase::Finish();
}

void AnalogToyEffectBase::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectBase::DeserializeFromXml(element);

   std::string text;
   int value;
   if (ReadXmlString(element, "ToyName", text))
      SetToyName(text);
   if (ReadXmlInt(element, "LayerNr", value))
      SetLayerNr(value);
}

}