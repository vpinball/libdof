#pragma once

#include "MatrixValueEffectBase.h"
#include "../../general/color/RGBAColor.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

class RGBAMatrixColorEffect : public MatrixValueEffectBase<RGBAColor>
{
public:
   RGBAMatrixColorEffect();
   virtual ~RGBAMatrixColorEffect() = default;

   virtual std::string GetXmlElementName() const override { return "RGBAMatrixColorEffect"; }

   const RGBAColor& GetActiveColor() const { return m_activeColor; }
   void SetActiveColor(const RGBAColor& value) { m_activeColor = value; }
   const RGBAColor& GetInactiveColor() const { return m_inactiveColor; }
   void SetInactiveColor(const RGBAColor& value) { m_inactiveColor = value; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual RGBAColor GetEffectValue(int triggerValue) override;

private:
   RGBAColor m_activeColor;
   RGBAColor m_inactiveColor;
};

}