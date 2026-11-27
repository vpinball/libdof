#pragma once

#include "../EffectEffectBase.h"
#include "../../table/TableElementData.h"
#include "../../pinballsupport/Action.h"
#include <functional>

namespace DOF
{

class DelayEffect : public EffectEffectBase
{
public:
   DelayEffect();
   virtual ~DelayEffect() = default;

   int GetDelayMs() const { return m_delayMs; }
   void SetDelayMs(int value);
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;

   virtual std::string GetXmlElementName() const override { return "DelayEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   void AfterDelay();

   int m_delayMs;
   Action m_afterDelayCallback;
   TableElementData m_delayTableElementData;
};

}