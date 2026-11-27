#pragma once

#include "RGBAEffectBase.h"
#include "../../general/color/RGBAColor.h"
#include "../FadeModeEnum.h"

namespace DOF
{

class TableElementData;

class RGBAColorEffect : public RGBAEffectBase
{
public:
   RGBAColorEffect();
   virtual ~RGBAColorEffect() = default;

   const RGBAColor& GetActiveColor() const { return m_activeColor; }
   void SetActiveColor(const RGBAColor& value) { m_activeColor = value; }
   const RGBAColor& GetInactiveColor() const { return m_inactiveColor; }
   void SetInactiveColor(const RGBAColor& value) { m_inactiveColor = value; }
   FadeModeEnum GetFadeMode() const { return m_fadeMode; }
   void SetFadeMode(FadeModeEnum value) { m_fadeMode = value; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;


   virtual std::string GetXmlElementName() const override { return "RGBAColorEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   RGBAColor m_activeColor;
   RGBAColor m_inactiveColor;
   FadeModeEnum m_fadeMode;
};

}