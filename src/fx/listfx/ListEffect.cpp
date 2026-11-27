#include "ListEffect.h"
#include "../../table/Table.h"
#include "../../table/TableElementData.h"

namespace DOF
{

ListEffect::ListEffect() { }

void ListEffect::Trigger(TableElementData* tableElementData) { m_assignedEffects.Trigger(*tableElementData); }

void ListEffect::Init(Table* table) { m_assignedEffects.Init(table); }

void ListEffect::Finish() { m_assignedEffects.Finish(); }

void ListEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectBase::DeserializeFromXml(element);

   const tinyxml2::XMLElement* assignedEffects = element->FirstChildElement("AssignedEffects");
   if (assignedEffects)
      m_assignedEffects.FromXml(assignedEffects);
}

}