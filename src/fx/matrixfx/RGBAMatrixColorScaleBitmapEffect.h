#pragma once

#include "MatrixBitmapEffectBase.h"
#include "../../general/color/RGBAColor.h"

namespace DOF
{

class RGBAMatrixColorScaleBitmapEffect : public MatrixBitmapEffectBase<RGBAColor>
{
public:
   RGBAMatrixColorScaleBitmapEffect();
   virtual ~RGBAMatrixColorScaleBitmapEffect() = default;

   const RGBAColor& GetActiveColor() const { return m_activeColor; }
   void SetActiveColor(const RGBAColor& value) { m_activeColor = value; }
   const RGBAColor& GetInactiveColor() const { return m_inactiveColor; }
   void SetInactiveColor(const RGBAColor& value) { m_inactiveColor = value; }

   virtual std::string GetXmlElementName() const override { return "RGBAMatrixColorScaleBitmapEffect"; }

   virtual void Init(Table* table) override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual RGBAColor GetEffectValue(int triggerValue, PixelData pixel) override;

private:
   RGBAColor m_activeColor;
   RGBAColor m_inactiveColor;
};

}