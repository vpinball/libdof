#pragma once

#include "../EffectBase.h"

namespace DOF
{

class Table;
class TableElementData;

class NullEffect : public EffectBase
{
public:
   NullEffect() = default;
   virtual ~NullEffect() = default;

   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;

   virtual std::string GetXmlElementName() const override { return "NullEffect"; }
};

}