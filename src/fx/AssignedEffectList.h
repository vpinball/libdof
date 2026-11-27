#pragma once

#include "AssignedEffect.h"
#include "DOF/DOF.h"

#include <tinyxml2/tinyxml2.h>

namespace DOF
{

class Table;
class TableElementData;

class AssignedEffectList : public std::vector<AssignedEffect*>
{
public:
   AssignedEffectList() = default;
   ~AssignedEffectList() { Finish(); }

   void Add(const std::string& effectName);
   void Trigger(TableElementData tableElementData);
   void Init(Table* table);
   void Finish();
   bool FromXml(const tinyxml2::XMLElement* element);
};

}
