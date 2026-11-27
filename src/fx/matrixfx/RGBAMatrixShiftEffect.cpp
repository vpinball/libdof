#include "RGBAMatrixShiftEffect.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

RGBAMatrixShiftEffect::RGBAMatrixShiftEffect()
   : m_activeColor(0xff, 0xff, 0xff, 0xff)
   , m_inactiveColor(0, 0, 0, 0)
{
}

RGBAColor RGBAMatrixShiftEffect::GetEffectValue(int triggerValue)
{
   RGBAColor d;

   int v = MathExtensions::Limit(triggerValue, 0, 255);
   d.SetRed(m_inactiveColor.GetRed() + MathExtensions::Limit((int)((float)(m_activeColor.GetRed() - m_inactiveColor.GetRed()) * v / 255), 0, 255));
   d.SetGreen(m_inactiveColor.GetGreen() + MathExtensions::Limit((int)((float)(m_activeColor.GetGreen() - m_inactiveColor.GetGreen()) * v / 255), 0, 255));
   d.SetBlue(m_inactiveColor.GetBlue() + MathExtensions::Limit((int)((float)(m_activeColor.GetBlue() - m_inactiveColor.GetBlue()) * v / 255), 0, 255));
   d.SetAlpha(m_inactiveColor.GetAlpha() + MathExtensions::Limit((int)((float)(m_activeColor.GetAlpha() - m_inactiveColor.GetAlpha()) * v / 255), 0, 255));

   return d;
}

void RGBAMatrixShiftEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixShiftEffectBase<RGBAColor>::DeserializeFromXml(element);

   RGBAColor color;
   if (ReadXmlRGBAColor(element, "ActiveColor", color))
      SetActiveColor(color);
   if (ReadXmlRGBAColor(element, "InactiveColor", color))
      SetInactiveColor(color);
}

}