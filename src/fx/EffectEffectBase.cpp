#include "EffectEffectBase.h"
#include "../table/Table.h"
#include "../table/TableElementData.h"
#include "EffectList.h"
#include "IEffect.h"
#include "../Log.h"
#include "../general/StringExtensions.h"
#include <stdexcept>

namespace DOF
{

EffectEffectBase::EffectEffectBase()
   : m_table(nullptr)
   , m_targetEffect(nullptr)
{
}

void EffectEffectBase::SetTargetEffectName(const std::string& value)
{
   if (m_targetEffectName != value)
   {
      m_targetEffectName = value;
      m_targetEffect = nullptr;
   }
}

void EffectEffectBase::TriggerTargetEffect(TableElementData* triggerData)
{
   if (m_targetEffect != nullptr)
   {
      try
      {
         TableElementData data = *triggerData;
         m_targetEffect->Trigger(&data);
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build(
            "The target effect {0} of the {1} {2} has thrown a exception. Disabling further calls of the target effect.", m_targetEffectName, "EffectEffectBase", NamedItemBase::GetName()));
         m_targetEffect = nullptr;
      }
   }
}

void EffectEffectBase::ResolveEffectName(Table* table)
{
   if (!StringExtensions::IsNullOrWhiteSpace(m_targetEffectName))
   {
      auto it = table->GetEffects()->find(m_targetEffectName);
      if (it != table->GetEffects()->end())
      {
         m_targetEffect = it->second;
      }
   }
}

void EffectEffectBase::Init(Table* table)
{
   m_table = table;
   ResolveEffectName(table);
}

void EffectEffectBase::Finish()
{
   m_targetEffect = nullptr;
   m_table = nullptr;
   EffectBase::Finish();
}

}