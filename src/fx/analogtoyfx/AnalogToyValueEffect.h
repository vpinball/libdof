#pragma once

#include "AnanlogToyEffectBase.h"
#include "../FadeModeEnum.h"
#include "../../general/analog/AnalogAlpha.h"

namespace DOF
{

class TableElementData;

class AnalogToyValueEffect : public AnalogToyEffectBase
{
public:
   AnalogToyValueEffect();
   virtual ~AnalogToyValueEffect() = default;

   const AnalogAlpha& GetActiveValue() const { return m_activeValue; }
   void SetActiveValue(const AnalogAlpha& value) { m_activeValue = value; }
   const AnalogAlpha& GetInactiveValue() const { return m_inactiveValue; }
   void SetInactiveValue(const AnalogAlpha& value) { m_inactiveValue = value; }
   FadeModeEnum GetFadeMode() const { return m_fadeMode; }
   void SetFadeMode(FadeModeEnum value) { m_fadeMode = value; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;


   virtual std::string GetXmlElementName() const override { return "AnalogToyValueEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   AnalogAlpha m_activeValue;
   AnalogAlpha m_inactiveValue;
   FadeModeEnum m_fadeMode;
};

}