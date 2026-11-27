#include "FadeEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../pinballsupport/Action.h"
#include "../../general/MathExtensions.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include <cmath>

namespace DOF
{

FadeEffect::FadeEffect()
   : m_fadeUpDuration(300)
   , m_fadeDownDuration(300)
   , m_fadeDurationMode(FadeEffectDurationModeEnum::CurrentToTarget)
   , m_targetValue(-1)
   , m_currentValue(0)
   , m_stepValue(0)
   , m_lastTargetTriggerValue(-1)
   , m_tableElementData()
   , m_fadingCallback(this, &FadeEffect::FadingStep)
{
}

FadeEffect::~FadeEffect() { }

void FadeEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr && tableElementData->m_value != m_targetValue)
   {
      m_targetValue = MathExtensions::Limit(tableElementData->m_value, 0, 255);

      m_tableElementData = *tableElementData;

      double duration = (m_currentValue < m_targetValue ? m_fadeUpDuration : m_fadeDownDuration);
      if (m_fadeDurationMode == FadeEffectDurationModeEnum::FullValueRange)
      {
         duration = duration / 255.0 * std::abs(m_targetValue - m_currentValue);
      }
      int steps = (int)(duration > 0 ? (duration / FadingRefreshIntervalMs) : 0);

      if (steps > 0)
      {
         m_stepValue = (float)(m_targetValue - m_currentValue) / steps;
         m_lastTargetTriggerValue = -1;
         FadingStep();
      }
      else
      {
         m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_fadingCallback);
         m_currentValue = m_targetValue;
         m_lastTargetTriggerValue = -1;
         TriggerTargetEffect(&m_tableElementData);
      }
   }
}

void FadeEffect::FadingStep()
{
   m_currentValue += m_stepValue;

   if ((m_currentValue < m_targetValue && m_stepValue > 0) || (m_currentValue > m_targetValue && m_stepValue < 0))
   {
      m_table->GetPinball()->GetAlarms()->RegisterIntervalAlarm(FadingRefreshIntervalMs, m_fadingCallback);
   }
   else
   {
      m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_fadingCallback);
      m_currentValue = m_targetValue;
   }

   if (m_lastTargetTriggerValue != (int)m_currentValue)
   {
      m_lastTargetTriggerValue = (int)m_currentValue;
      m_tableElementData.m_value = m_lastTargetTriggerValue;
      TriggerTargetEffect(&m_tableElementData);
   }
}

void FadeEffect::Finish()
{
   try
   {
      m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_fadingCallback);
   }
   catch (...)
   {
   }

   EffectEffectBase::Finish();
}

void FadeEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectEffectBase::DeserializeFromXml(element);

   std::string text;
   int value;
   if (ReadXmlInt(element, "FadeUpDuration", value))
      SetFadeUpDuration(value);
   if (ReadXmlInt(element, "FadeDownDuration", value))
      SetFadeDownDuration(value);
   if (ReadXmlString(element, "FadeDurationMode", text))
   {
      if (text == "CurrentToTarget")
         SetFadeDurationMode(FadeEffectDurationModeEnum::CurrentToTarget);
      else if (text == "FullValueRange")
         SetFadeDurationMode(FadeEffectDurationModeEnum::FullValueRange);
      else
         ThrowInvalidXmlValue("FadeDurationMode", text);
   }
}

}