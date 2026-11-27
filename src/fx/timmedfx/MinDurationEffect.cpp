#include "MinDurationEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"

namespace DOF
{

MinDurationEffect::MinDurationEffect()
   : m_retriggerBehaviour(RetriggerBehaviourEnum::Restart)
   , m_minDurationMs(500)
   , m_active(false)
   , m_untriggerData()
   , m_durationStart()
   , m_minDurationEndCallback(this, &MinDurationEffect::MinDurationEnd)
{
}

void MinDurationEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr)
   {
      if (tableElementData->m_value != 0)
      {
         if (!m_active || m_retriggerBehaviour == RetriggerBehaviourEnum::Restart)
         {
            m_durationStart = std::chrono::steady_clock::now();
            m_untriggerData = *tableElementData;
            TriggerTargetEffect(tableElementData);
            m_active = true;
         }
      }
      else
      {
         if (m_active && m_untriggerData.m_tableElementType == tableElementData->m_tableElementType && m_untriggerData.m_number == tableElementData->m_number)
         {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_durationStart).count();
            if (elapsed >= m_minDurationMs)
            {
               MinDurationEnd();
            }
            else
            {
               int remainingMs = m_minDurationMs - (int)elapsed;
               m_table->GetPinball()->GetAlarms()->RegisterAlarm(remainingMs, m_minDurationEndCallback);
            }
         }
      }
   }
}

void MinDurationEffect::MinDurationEnd()
{
   if (m_active)
   {
      TableElementData d = m_untriggerData;
      d.m_value = 0;
      TriggerTargetEffect(&d);
   }
   m_active = false;
}

void MinDurationEffect::Finish()
{
   try
   {
      m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_minDurationEndCallback);
   }
   catch (...)
   {
   }
   m_active = false;
   EffectEffectBase::Finish();
}

void MinDurationEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectEffectBase::DeserializeFromXml(element);

   int value;
   RetriggerBehaviourEnum retriggerBehaviour;
   if (ReadXmlRetriggerBehaviour(element, "RetriggerBehaviour", retriggerBehaviour))
      SetRetriggerBehaviour(retriggerBehaviour);
   if (ReadXmlInt(element, "MinDurationMs", value))
      SetMinDurationMs(value);
}

}