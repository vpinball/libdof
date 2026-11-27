#include "BlinkEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../general/MathExtensions.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include <climits>

namespace DOF
{

BlinkEffect::BlinkEffect()
   : m_activeValue(-1)
   , m_lowValue(0)
   , m_durationActiveMs(500)
   , m_durationInactiveMs(500)
   , m_untriggerBehaviour(BlinkEffectUntriggerBehaviourEnum::Immediate)
   , m_active(false)
   , m_blinkEnabled(false)
   , m_blinkState(false)
   , m_blinkOrgTableElementDataValue(1)
   , m_blinkTableElementData()
   , m_alarmCallback(this, &BlinkEffect::DoBlink)
{
}

BlinkEffect::~BlinkEffect() { }

void BlinkEffect::SetHighValue(int value) { m_activeValue = MathExtensions::Limit(value, -1, 255); }

void BlinkEffect::SetLowValue(int value) { m_lowValue = MathExtensions::Limit(value, 0, 255); }

void BlinkEffect::SetDurationActiveMs(int value) { m_durationActiveMs = MathExtensions::Limit(value, 1, INT_MAX); }

void BlinkEffect::SetDurationInactiveMs(int value) { m_durationInactiveMs = MathExtensions::Limit(value, 1, INT_MAX); }

void BlinkEffect::StartBlinking(TableElementData* tableElementData)
{
   m_blinkTableElementData = *tableElementData;
   m_blinkOrgTableElementDataValue = m_blinkTableElementData.m_value;

   if (!m_blinkEnabled)
   {
      m_blinkEnabled = true;
      m_blinkState = false;
      DoBlink();
   }
   else
   {
      if (m_blinkState)
      {
         m_blinkTableElementData.m_value = (m_activeValue >= 0 ? m_activeValue : m_blinkOrgTableElementDataValue);
         TriggerTargetEffect(&m_blinkTableElementData);
      }
   }
}

void BlinkEffect::StopBlinking()
{
   if (m_blinkEnabled)
   {
      m_blinkEnabled = false;
      if (m_untriggerBehaviour == BlinkEffectUntriggerBehaviourEnum::Immediate)
      {
         m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_alarmCallback);
         m_blinkTableElementData.m_value = 0;
         TriggerTargetEffect(&m_blinkTableElementData);
      }
   }
}

void BlinkEffect::DoBlink()
{
   m_blinkState = !m_blinkState;
   if (m_blinkState)
   {
      m_blinkTableElementData.m_value = (m_activeValue >= 0 ? m_activeValue : m_blinkOrgTableElementDataValue);
      m_table->GetPinball()->GetAlarms()->RegisterAlarm(m_durationActiveMs, m_alarmCallback);
   }
   else
   {
      if (m_blinkEnabled)
      {
         m_blinkTableElementData.m_value = m_lowValue;
         m_table->GetPinball()->GetAlarms()->RegisterAlarm(m_durationInactiveMs, m_alarmCallback);
      }
      else
         m_blinkTableElementData.m_value = 0;
   }
   TriggerTargetEffect(&m_blinkTableElementData);
}

void BlinkEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr)
   {
      if (tableElementData->m_value != 0)
         StartBlinking(tableElementData);
      else
         StopBlinking();
   }
}

void BlinkEffect::Finish()
{
   try
   {
      m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_alarmCallback);
   }
   catch (...)
   {
   }
   EffectEffectBase::Finish();
}

void BlinkEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectEffectBase::DeserializeFromXml(element);

   std::string text;
   int value;
   if (ReadXmlInt(element, "HighValue", value))
      SetHighValue(value);
   if (ReadXmlInt(element, "LowValue", value))
      SetLowValue(value);
   if (ReadXmlInt(element, "DurationActiveMs", value))
      SetDurationActiveMs(value);
   if (ReadXmlInt(element, "DurationInactiveMs", value))
      SetDurationInactiveMs(value);
   if (ReadXmlString(element, "UntriggerBehaviour", text))
   {
      if (text == "Immediate")
         SetUntriggerBehaviour(BlinkEffectUntriggerBehaviourEnum::Immediate);
      else if (text == "CompleteHigh")
         SetUntriggerBehaviour(BlinkEffectUntriggerBehaviourEnum::CompleteHigh);
      else
         ThrowInvalidXmlValue("UntriggerBehaviour", text);
   }
}

}