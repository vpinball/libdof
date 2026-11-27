#pragma once

#include "../EffectEffectBase.h"
#include "../RetriggerBehaviourEnum.h"
#include "../../table/TableElementData.h"
#include "../../pinballsupport/Action.h"

namespace DOF
{

class MaxDurationEffect : public EffectEffectBase
{
public:
   MaxDurationEffect();
   virtual ~MaxDurationEffect() = default;

   RetriggerBehaviourEnum GetRetriggerBehaviour() const { return m_retriggerBehaviour; }
   void SetRetriggerBehaviour(RetriggerBehaviourEnum value) { m_retriggerBehaviour = value; }
   int GetMaxDurationMs() const { return m_maxDurationMs; }
   void SetMaxDurationMs(int value);
   bool GetActive() const { return m_active; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Finish() override;
   virtual std::string GetXmlElementName() const override { return "MaxDurationEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   void DurationEnd();

   RetriggerBehaviourEnum m_retriggerBehaviour;
   int m_maxDurationMs;
   bool m_active;
   TableElementData m_untriggerData;
   Action m_durationEndCallback;
};

}