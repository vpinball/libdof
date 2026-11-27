#include "RGBAEffectBase.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../cab/Cabinet.h"
#include "../../cab/toys/ToyList.h"
#include "../../cab/toys/IRGBAToy.h"
#include "../../general/StringExtensions.h"

namespace DOF
{

RGBAEffectBase::RGBAEffectBase()
   : m_table(nullptr)
   , m_rgbaToy(nullptr)
   , m_layer(nullptr)
   , m_layerNr(0)
{
}

void RGBAEffectBase::SetToyName(const std::string& value)
{
   if (m_toyName != value)
   {
      m_toyName = value;
      m_rgbaToy = nullptr;
      m_layer = nullptr;
   }
}

void RGBAEffectBase::Init(Table* table)
{
   m_table = table;
   if (!StringExtensions::IsNullOrWhiteSpace(m_toyName) && m_table->GetPinball()->GetCabinet()->GetToys()->Contains(m_toyName))
   {
      IToy* toy = m_table->GetPinball()->GetCabinet()->GetToys()->FindByName(m_toyName);
      m_rgbaToy = dynamic_cast<IRGBAToy*>(toy);
      if (m_rgbaToy != nullptr)
         m_layer = m_rgbaToy->GetLayers().GetOrCreateLayer(m_layerNr, 1);
   }
}

void RGBAEffectBase::Finish()
{
   m_layer = nullptr;
   m_rgbaToy = nullptr;
   m_table = nullptr;

   EffectBase::Finish();
}

void RGBAEffectBase::DeserializeFromXml(const tinyxml2::XMLElement* element)
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