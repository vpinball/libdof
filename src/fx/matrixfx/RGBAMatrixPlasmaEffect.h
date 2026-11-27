#pragma once

#include "MatrixPlasmaEffectBase.h"
#include "../../general/color/RGBAColor.h"

namespace DOF
{

class RGBAMatrixPlasmaEffect : public MatrixPlasmaEffectBase<RGBAColor>
{
public:
   RGBAMatrixPlasmaEffect();
   virtual ~RGBAMatrixPlasmaEffect() = default;

   const RGBAColor& GetActiveColor1() const { return m_activeColor1; }
   void SetActiveColor1(const RGBAColor& value) { m_activeColor1 = value; }
   const RGBAColor& GetActiveColor2() const { return m_activeColor2; }
   void SetActiveColor2(const RGBAColor& value) { m_activeColor2 = value; }
   const RGBAColor& GetInactiveColor() const { return m_inactiveColor; }
   void SetInactiveColor(const RGBAColor& value) { m_inactiveColor = value; }

   virtual std::string GetXmlElementName() const override { return "RGBAMatrixPlasmaEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual RGBAColor GetEffectValue(int triggerValue, double time, double value, double x, double y) override;

private:
   RGBAColor m_activeColor1;
   RGBAColor m_activeColor2;
   RGBAColor m_inactiveColor;
};

}
