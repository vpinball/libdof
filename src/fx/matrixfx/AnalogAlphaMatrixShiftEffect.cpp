#include "AnalogAlphaMatrixShiftEffect.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

AnalogAlphaMatrixShiftEffect::AnalogAlphaMatrixShiftEffect()
   : m_activeValue(255, 255)
   , m_inactiveValue(0, 0)
{
}

AnalogAlpha AnalogAlphaMatrixShiftEffect::GetEffectValue(int triggerValue)
{
   AnalogAlpha d;

   int v = MathExtensions::Limit(triggerValue, 0, 255);
   d.SetValue(m_inactiveValue.GetValue() + MathExtensions::Limit((int)((float)(m_activeValue.GetValue() - m_inactiveValue.GetValue()) * v / 255), 0, 255));
   d.SetAlpha(m_inactiveValue.GetAlpha() + MathExtensions::Limit((int)((float)(m_activeValue.GetAlpha() - m_inactiveValue.GetAlpha()) * v / 255), 0, 255));

   return d;
}

void AnalogAlphaMatrixShiftEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixShiftEffectBase<AnalogAlpha>::DeserializeFromXml(element);

   AnalogAlpha analogAlpha;
   if (ReadXmlAnalogAlpha(element, "ActiveValue", analogAlpha))
      SetActiveValue(analogAlpha);
   if (ReadXmlAnalogAlpha(element, "InactiveValue", analogAlpha))
      SetInactiveValue(analogAlpha);
}

}