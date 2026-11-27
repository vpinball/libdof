#include "RGBAMatrixColorEffect.h"
#include "../../general/MathExtensions.h"
#include "../../Log.h"
#include "../../general/StringExtensions.h"

namespace DOF
{

RGBAMatrixColorEffect::RGBAMatrixColorEffect()
   : m_activeColor(0xff, 0xff, 0xff, 0xff)
   , m_inactiveColor(0, 0, 0, 0)
{
}

RGBAColor RGBAMatrixColorEffect::GetEffectValue(int triggerValue)
{
   RGBAColor d;

   int v = MathExtensions::Limit(triggerValue, 0, 255);
   d.SetRed(m_inactiveColor.GetRed() + MathExtensions::Limit((int)((float)(m_activeColor.GetRed() - m_inactiveColor.GetRed()) * v / 255), 0, 255));
   d.SetGreen(m_inactiveColor.GetGreen() + MathExtensions::Limit((int)((float)(m_activeColor.GetGreen() - m_inactiveColor.GetGreen()) * v / 255), 0, 255));
   d.SetBlue(m_inactiveColor.GetBlue() + MathExtensions::Limit((int)((float)(m_activeColor.GetBlue() - m_inactiveColor.GetBlue()) * v / 255), 0, 255));
   d.SetAlpha(m_inactiveColor.GetAlpha() + MathExtensions::Limit((int)((float)(m_activeColor.GetAlpha() - m_inactiveColor.GetAlpha()) * v / 255), 0, 255));

   return d;
}

void RGBAMatrixColorEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixValueEffectBase<RGBAColor>::DeserializeFromXml(element);

   RGBAColor color;
   if (ReadXmlRGBAColor(element, "ActiveColor", color))
      SetActiveColor(color);
   if (ReadXmlRGBAColor(element, "InactiveColor", color))
      SetInactiveColor(color);
}

}