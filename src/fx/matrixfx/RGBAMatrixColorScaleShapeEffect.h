#pragma once

#include "MatrixEffectBase.h"
#include "../../general/color/RGBAColor.h"
#include <string>

namespace DOF
{

class TableElementData;
class Table;
class IEffect;

class RGBAMatrixColorScaleShapeEffect : public MatrixEffectBase<RGBAColor>
{
private:
   RGBAColor m_activeColor;
   RGBAColor m_inactiveColor;
   std::string m_shapeName;
   IEffect* m_targetEffect;

public:
   RGBAMatrixColorScaleShapeEffect();
   virtual ~RGBAMatrixColorScaleShapeEffect();

   const RGBAColor& GetActiveColor() const { return m_activeColor; }
   void SetActiveColor(const RGBAColor& value) { m_activeColor = value; }
   const RGBAColor& GetInactiveColor() const { return m_inactiveColor; }
   void SetInactiveColor(const RGBAColor& value) { m_inactiveColor = value; }

   const std::string& GetShapeName() const { return m_shapeName; }
   void SetShapeName(const std::string& value) { m_shapeName = value; }

   virtual void Init(Table* table) override;
   virtual void Finish() override;
   virtual void Trigger(TableElementData* tableElementData) override;

   virtual std::string GetXmlElementName() const override { return "RGBAMatrixColorScaleShapeEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;
};

}