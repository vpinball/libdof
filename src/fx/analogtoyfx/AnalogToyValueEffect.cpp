#include "AnalogToyValueEffect.h"
#include "../../table/TableElementData.h"
#include "../../general/MathExtensions.h"
#include "../../Log.h"
#include "../../general/StringExtensions.h"

namespace DOF
{

AnalogToyValueEffect::AnalogToyValueEffect()
   : m_activeValue(255, 255)
   , m_inactiveValue(0, 0)
   , m_fadeMode(FadeModeEnum::Fade)
{
}

void AnalogToyValueEffect::Trigger(TableElementData* tableElementData)
{
   if (m_layer != nullptr)
   {
      int fadeValue = tableElementData->m_value;
      if (m_fadeMode == FadeModeEnum::OnOff && fadeValue > 0)
         fadeValue = 255;

      int newValue = m_inactiveValue.GetValue() + MathExtensions::Limit((int)((float)(m_activeValue.GetValue() - m_inactiveValue.GetValue()) * fadeValue / 255), 0, 255);
      int newAlpha = m_inactiveValue.GetAlpha() + MathExtensions::Limit((int)((float)(m_activeValue.GetAlpha() - m_inactiveValue.GetAlpha()) * fadeValue / 255), 0, 255);

      m_layer->SetValue(newValue);
      m_layer->SetAlpha(newAlpha);
   }
}

void AnalogToyValueEffect::Init(Table* table) { AnalogToyEffectBase::Init(table); }

void AnalogToyValueEffect::Finish() { AnalogToyEffectBase::Finish(); }

void AnalogToyValueEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   AnalogToyEffectBase::DeserializeFromXml(element);

   AnalogAlpha analogAlpha;
   FadeModeEnum fadeMode;
   if (ReadXmlAnalogAlpha(element, "ActiveValue", analogAlpha))
      SetActiveValue(analogAlpha);
   if (ReadXmlAnalogAlpha(element, "InactiveValue", analogAlpha))
      SetInactiveValue(analogAlpha);
   if (ReadXmlFadeMode(element, "FadeMode", fadeMode))
      SetFadeMode(fadeMode);
}

}