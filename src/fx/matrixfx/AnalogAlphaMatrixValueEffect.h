#pragma once

#include "MatrixValueEffectBase.h"
#include "../../general/analog/AnalogAlpha.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

class AnalogAlphaMatrixValueEffect : public MatrixValueEffectBase<AnalogAlpha>
{
public:
   AnalogAlphaMatrixValueEffect();
   virtual ~AnalogAlphaMatrixValueEffect() = default;

   virtual std::string GetXmlElementName() const override { return "AnalogAlphaMatrixValueEffect"; }

   const AnalogAlpha& GetActiveValue() const { return m_activeValue; }
   void SetActiveValue(const AnalogAlpha& value) { m_activeValue = value; }
   const AnalogAlpha& GetInactiveValue() const { return m_inactiveValue; }
   void SetInactiveValue(const AnalogAlpha& value) { m_inactiveValue = value; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual AnalogAlpha GetEffectValue(int triggerValue) override;

private:
   AnalogAlpha m_activeValue;
   AnalogAlpha m_inactiveValue;
};

}