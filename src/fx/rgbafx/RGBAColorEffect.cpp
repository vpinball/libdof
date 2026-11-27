#include "RGBAColorEffect.h"
#include "../../table/TableElementData.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

RGBAColorEffect::RGBAColorEffect()
   : m_activeColor(255, 255, 255, 255)
   , m_inactiveColor(0, 0, 0, 0)
   , m_fadeMode(FadeModeEnum::Fade)
{
}

void RGBAColorEffect::Trigger(TableElementData* tableElementData)
{
   if (m_layer != nullptr)
   {
      int fadeValue = tableElementData->m_value;
      if (m_fadeMode == FadeModeEnum::OnOff && fadeValue > 0)
      {
         fadeValue = 255;
      }

      m_layer->SetRed(m_inactiveColor.GetRed() + MathExtensions::Limit((int)((float)(m_activeColor.GetRed() - m_inactiveColor.GetRed()) * fadeValue / 255), 0, 255));
      m_layer->SetGreen(m_inactiveColor.GetGreen() + MathExtensions::Limit((int)((float)(m_activeColor.GetGreen() - m_inactiveColor.GetGreen()) * fadeValue / 255), 0, 255));
      m_layer->SetBlue(m_inactiveColor.GetBlue() + MathExtensions::Limit((int)((float)(m_activeColor.GetBlue() - m_inactiveColor.GetBlue()) * fadeValue / 255), 0, 255));
      m_layer->SetAlpha(m_inactiveColor.GetAlpha() + MathExtensions::Limit((int)((float)(m_activeColor.GetAlpha() - m_inactiveColor.GetAlpha()) * fadeValue / 255), 0, 255));
   }
}

void RGBAColorEffect::Init(Table* table) { RGBAEffectBase::Init(table); }

void RGBAColorEffect::Finish() { RGBAEffectBase::Finish(); }

void RGBAColorEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   RGBAEffectBase::DeserializeFromXml(element);

   RGBAColor color;
   FadeModeEnum fadeMode;
   if (ReadXmlRGBAColor(element, "ActiveColor", color))
      SetActiveColor(color);
   if (ReadXmlRGBAColor(element, "InactiveColor", color))
      SetInactiveColor(color);
   if (ReadXmlFadeMode(element, "FadeMode", fadeMode))
      SetFadeMode(fadeMode);
}

}