#pragma once

#include "../EffectEffectBase.h"
#include "BlinkEffectUntriggerBehaviourEnum.h"
#include "../../table/TableElementData.h"
#include "../../pinballsupport/Action.h"
#include <functional>

namespace DOF
{

class BlinkEffect : public EffectEffectBase
{
public:
   BlinkEffect();
   virtual ~BlinkEffect();

   int GetHighValue() const { return m_activeValue; }
   void SetHighValue(int value);
   int GetLowValue() const { return m_lowValue; }
   void SetLowValue(int value);
   int GetDurationActiveMs() const { return m_durationActiveMs; }
   void SetDurationActiveMs(int value);
   int GetDurationInactiveMs() const { return m_durationInactiveMs; }
   void SetDurationInactiveMs(int value);
   BlinkEffectUntriggerBehaviourEnum GetUntriggerBehaviour() const { return m_untriggerBehaviour; }
   void SetUntriggerBehaviour(BlinkEffectUntriggerBehaviourEnum value) { m_untriggerBehaviour = value; }
   bool GetActive() const { return m_active; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Finish() override;


   virtual std::string GetXmlElementName() const override { return "BlinkEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   void StartBlinking(TableElementData* tableElementData);
   void StopBlinking();
   void DoBlink();

   int m_activeValue;
   int m_lowValue;
   int m_durationActiveMs;
   int m_durationInactiveMs;
   BlinkEffectUntriggerBehaviourEnum m_untriggerBehaviour;
   bool m_active;
   bool m_blinkEnabled;
   bool m_blinkState;
   int m_blinkOrgTableElementDataValue;
   TableElementData m_blinkTableElementData;

   Action m_alarmCallback;
};

}