#pragma once

#include "MatrixShiftEffectBase.h"
#include "../../general/color/RGBAColor.h"

namespace DOF
{

class RGBAMatrixShiftEffect : public MatrixShiftEffectBase<RGBAColor>
{
public:
   RGBAMatrixShiftEffect();
   virtual ~RGBAMatrixShiftEffect() = default;

   virtual std::string GetXmlElementName() const override { return "RGBAMatrixShiftEffect"; }

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