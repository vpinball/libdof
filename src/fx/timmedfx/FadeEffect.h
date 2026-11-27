#pragma once

#include "../EffectEffectBase.h"
#include "FadeEffectDurationModeEnum.h"
#include "../../table/TableElementData.h"
#include "../../pinballsupport/Action.h"

namespace DOF
{

class TableElementData;

class FadeEffect : public EffectEffectBase
{
public:
   FadeEffect();
   virtual ~FadeEffect();

   int GetFadeUpDuration() const { return m_fadeUpDuration; }
   void SetFadeUpDuration(int value) { m_fadeUpDuration = value; }
   int GetFadeDownDuration() const { return m_fadeDownDuration; }
   void SetFadeDownDuration(int value) { m_fadeDownDuration = value; }
   FadeEffectDurationModeEnum GetFadeDurationMode() const { return m_fadeDurationMode; }
   void SetFadeDurationMode(FadeEffectDurationModeEnum value) { m_fadeDurationMode = value; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Finish() override;

   virtual std::string GetXmlElementName() const override { return "FadeEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   static const int FadingRefreshIntervalMs = 30;
   void FadingStep();

   int m_fadeUpDuration;
   int m_fadeDownDuration;
   FadeEffectDurationModeEnum m_fadeDurationMode;
   float m_targetValue;
   float m_currentValue;
   float m_stepValue;
   int m_lastTargetTriggerValue;
   TableElementData m_tableElementData;
   Action m_fadingCallback;
};

}