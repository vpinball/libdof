#include "RGBAMatrixPlasmaEffect.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

RGBAMatrixPlasmaEffect::RGBAMatrixPlasmaEffect()
   : m_activeColor1(0x00, 0x00, 0xff, 0xff)
   , m_activeColor2(0x80, 0x80, 0x00, 0xff)
   , m_inactiveColor(0, 0, 0, 0)
{
}

RGBAColor RGBAMatrixPlasmaEffect::GetEffectValue(int triggerValue, double time, double value, double x, double y)
{
   double blendVal = (std::sin(value * M_PI * 2 + time) + 1) / 2;
   RGBAColor blended((int)(blendVal * m_activeColor1.GetRed() + (1 - blendVal) * m_activeColor2.GetRed()),
      (int)(blendVal * m_activeColor1.GetGreen() + (1 - blendVal) * m_activeColor2.GetGreen()), (int)(blendVal * m_activeColor1.GetBlue() + (1 - blendVal) * m_activeColor2.GetBlue()),
      (int)((blendVal * m_activeColor1.GetAlpha() + (1 - blendVal) * m_activeColor2.GetAlpha()) * value));

   RGBAColor d;

   int v = MathExtensions::Limit(triggerValue, 0, 255);
   d.SetRed(m_inactiveColor.GetRed() + MathExtensions::Limit((int)((float)(blended.GetRed() - m_inactiveColor.GetRed()) * v / 255), 0, 255));
   d.SetGreen(m_inactiveColor.GetGreen() + MathExtensions::Limit((int)((float)(blended.GetGreen() - m_inactiveColor.GetGreen()) * v / 255), 0, 255));
   d.SetBlue(m_inactiveColor.GetBlue() + MathExtensions::Limit((int)((float)(blended.GetBlue() - m_inactiveColor.GetBlue()) * v / 255), 0, 255));
   d.SetAlpha(m_inactiveColor.GetAlpha() + MathExtensions::Limit((int)((float)(blended.GetAlpha() - m_inactiveColor.GetAlpha()) * v / 255), 0, 255));

   return d;
}

void RGBAMatrixPlasmaEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixPlasmaEffectBase<RGBAColor>::DeserializeFromXml(element);

   RGBAColor color;
   if (ReadXmlRGBAColor(element, "ActiveColor1", color))
      SetActiveColor1(color);
   if (ReadXmlRGBAColor(element, "ActiveColor2", color))
      SetActiveColor2(color);
   if (ReadXmlRGBAColor(element, "InactiveColor", color))
      SetInactiveColor(color);
}

}
