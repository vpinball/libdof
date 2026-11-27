#pragma once

#include "../EffectEffectBase.h"
#include "../../table/TableElementData.h"
#include "../../pinballsupport/Action.h"

namespace DOF
{

class ExtendDurationEffect : public EffectEffectBase
{
public:
   ExtendDurationEffect();
   virtual ~ExtendDurationEffect() = default;

   int GetDurationMs() const { return m_durationMs; }
   void SetDurationMs(int value);
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Finish() override;
   virtual std::string GetXmlElementName() const override { return "ExtendDurationEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   void ExtendedDurationEnd();

   int m_durationMs;
   TableElementData m_delayedData;
   Action m_extendedDurationEndCallback;
};

}