#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include "DOF/DOF.h"
#include "../general/generic/IXmlSerializable.h"

namespace DOF
{

class IEffect;
class Table;

class EffectList : public std::unordered_map<std::string, IEffect*>, public IXmlSerializable
{
public:
   EffectList() = default;
   ~EffectList();

   void Finish();
   void Init(Table* table);


   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "Effects"; }


   void Add(IEffect* effect);
   bool Contains(const std::string& name) const;

private:
   std::unordered_set<IEffect*> m_initialized;
};

}
