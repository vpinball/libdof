#include "AnalogAlphaMatrixValueEffect.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

AnalogAlphaMatrixValueEffect::AnalogAlphaMatrixValueEffect()
   : m_activeValue(0xff, 0xff)
   , m_inactiveValue(0, 0)
{
}

AnalogAlpha AnalogAlphaMatrixValueEffect::GetEffectValue(int triggerValue)
{
   AnalogAlpha d;

   int v = MathExtensions::Limit(triggerValue, 0, 255);
   d.SetValue(m_inactiveValue.GetValue() + MathExtensions::Limit((int)((float)(m_activeValue.GetValue() - m_inactiveValue.GetValue()) * v / 255), 0, 255));
   d.SetAlpha(m_inactiveValue.GetAlpha() + MathExtensions::Limit((int)((float)(m_activeValue.GetAlpha() - m_inactiveValue.GetAlpha()) * v / 255), 0, 255));

   return d;
}

void AnalogAlphaMatrixValueEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixValueEffectBase<AnalogAlpha>::DeserializeFromXml(element);

   AnalogAlpha analogAlpha;
   if (ReadXmlAnalogAlpha(element, "ActiveValue", analogAlpha))
      SetActiveValue(analogAlpha);
   if (ReadXmlAnalogAlpha(element, "InactiveValue", analogAlpha))
      SetInactiveValue(analogAlpha);
}

}