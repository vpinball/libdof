#pragma once

#include "EffectBase.h"
#include <string>

namespace DOF
{

class Table;
class IEffect;
class TableElementData;

class EffectEffectBase : public EffectBase
{
public:
   EffectEffectBase();
   virtual ~EffectEffectBase() = default;

   const std::string& GetTargetEffectName() const { return m_targetEffectName; }
   void SetTargetEffectName(const std::string& value);

   virtual void Init(Table* table) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   Table* m_table;
   IEffect* m_targetEffect;

   void TriggerTargetEffect(TableElementData* triggerData);

private:
   std::string m_targetEffectName;

   void ResolveEffectName(Table* table);
};

}