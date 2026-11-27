#include "ExtendDurationEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../general/MathExtensions.h"
#include <climits>

namespace DOF
{

ExtendDurationEffect::ExtendDurationEffect()
   : m_durationMs(500)
   , m_delayedData()
   , m_extendedDurationEndCallback(this, &ExtendDurationEffect::ExtendedDurationEnd)
{
}

void ExtendDurationEffect::SetDurationMs(int value) { m_durationMs = MathExtensions::Limit(value, 0, INT_MAX); }

void ExtendDurationEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr)
   {
      if (tableElementData->m_value != 0)
      {
         TriggerTargetEffect(tableElementData);
      }
      else
      {
         if (m_durationMs > 0)
         {
            m_delayedData = *tableElementData;
            m_table->GetPinball()->GetAlarms()->RegisterAlarm(m_durationMs, m_extendedDurationEndCallback, true);
         }
         else
         {
            TriggerTargetEffect(tableElementData);
         }
      }
   }
}

void ExtendDurationEffect::ExtendedDurationEnd() { TriggerTargetEffect(&m_delayedData); }

void ExtendDurationEffect::Finish()
{
   try
   {
      m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_extendedDurationEndCallback);
   }
   catch (...)
   {
   }
   EffectEffectBase::Finish();
}

void ExtendDurationEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectEffectBase::DeserializeFromXml(element);

   int value;
   if (ReadXmlInt(element, "DurationMs", value))
      SetDurationMs(value);
}

}