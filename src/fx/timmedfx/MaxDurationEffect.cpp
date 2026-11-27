#include "MaxDurationEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../general/MathExtensions.h"
#include <climits>

namespace DOF
{

MaxDurationEffect::MaxDurationEffect()
   : m_retriggerBehaviour(RetriggerBehaviourEnum::Restart)
   , m_maxDurationMs(500)
   , m_active(false)
   , m_untriggerData()
   , m_durationEndCallback(this, &MaxDurationEffect::DurationEnd)
{
}

void MaxDurationEffect::SetMaxDurationMs(int value) { m_maxDurationMs = MathExtensions::Limit(value, 1, INT_MAX); }

void MaxDurationEffect::Trigger(TableElementData* tableElementData)
{
   if (tableElementData->m_value != 0)
   {
      if (!m_active || m_retriggerBehaviour == RetriggerBehaviourEnum::Restart)
      {
         TriggerTargetEffect(tableElementData);
         m_untriggerData = *tableElementData;
         m_untriggerData.m_value = 0;
         m_table->GetPinball()->GetAlarms()->RegisterAlarm(m_maxDurationMs, m_durationEndCallback);
         m_active = true;
      }
   }
   else
   {
      if (m_active && m_untriggerData.m_tableElementType == tableElementData->m_tableElementType && m_untriggerData.m_number == tableElementData->m_number)
      {
         TriggerTargetEffect(tableElementData);
         m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_durationEndCallback);
         m_active = false;
      }
   }
}

void MaxDurationEffect::DurationEnd()
{
   TriggerTargetEffect(&m_untriggerData);
   m_active = false;
}

void MaxDurationEffect::Finish()
{
   try
   {
      if (m_table && m_table->GetPinball() && m_table->GetPinball()->GetAlarms())
      {
         m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_durationEndCallback);
      }
   }
   catch (...)
   {
   }
   m_active = false;
   EffectEffectBase::Finish();
}

void MaxDurationEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectEffectBase::DeserializeFromXml(element);

   int value;
   RetriggerBehaviourEnum retriggerBehaviour;
   if (ReadXmlRetriggerBehaviour(element, "RetriggerBehaviour", retriggerBehaviour))
      SetRetriggerBehaviour(retriggerBehaviour);
   if (ReadXmlInt(element, "MaxDurationMs", value))
      SetMaxDurationMs(value);
}

}