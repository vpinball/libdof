#pragma once

#include "MatrixEffectBase.h"
#include "../../general/color/RGBAColor.h"
#include <string>

namespace DOF
{

class TableElementData;
class Table;
class IMatrixBitmapEffect;
class RGBAMatrixBitmapEffect;

class RGBAMatrixShapeEffect : public MatrixEffectBase<RGBAColor>
{
private:
   std::string m_shapeName;
   IMatrixBitmapEffect* m_targetEffect;

public:
   RGBAMatrixShapeEffect();
   virtual ~RGBAMatrixShapeEffect();

   const std::string& GetShapeName() const;
   void SetShapeName(const std::string& value);

   virtual void Init(Table* table) override;
   virtual void Finish() override;
   virtual void Trigger(TableElementData* tableElementData) override;

   virtual std::string GetXmlElementName() const override { return "RGBAMatrixShapeEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;
};

}