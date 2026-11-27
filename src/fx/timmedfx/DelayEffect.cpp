#include "DelayEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../general/MathExtensions.h"
#include <climits>

namespace DOF
{

DelayEffect::DelayEffect()
   : m_delayMs(0)
   , m_delayTableElementData()
   , m_afterDelayCallback(this, &DelayEffect::AfterDelay)
{
}

void DelayEffect::SetDelayMs(int value) { m_delayMs = MathExtensions::Limit(value, 0, INT_MAX); }

void DelayEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr)
   {
      if (m_delayMs > 0)
      {
         m_delayTableElementData = *tableElementData;
         m_table->GetPinball()->GetAlarms()->RegisterAlarm(m_delayMs, m_afterDelayCallback, true);
      }
      else
      {
         TriggerTargetEffect(tableElementData);
      }
   }
}

void DelayEffect::AfterDelay() { TriggerTargetEffect(&m_delayTableElementData); }

void DelayEffect::Init(Table* table) { EffectEffectBase::Init(table); }

void DelayEffect::Finish()
{
   try
   {
      m_table->GetPinball()->GetAlarms()->UnregisterAlarm(m_afterDelayCallback);
   }
   catch (...)
   {
   }
   EffectEffectBase::Finish();
}

void DelayEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectEffectBase::DeserializeFromXml(element);

   int value;
   if (ReadXmlInt(element, "DelayMs", value))
      SetDelayMs(value);
}

}