#pragma once

#include "DOF/DOF.h"
#include "IOutputController.h"
#include "../../general/generic/IXmlSerializable.h"

namespace DOF
{

class OutputControllerList : public std::vector<IOutputController*>, public IXmlSerializable
{
public:
   OutputControllerList();
   virtual ~OutputControllerList();

   void Init(Cabinet* cabinet);
   void Finish();
   void Update();
   bool Contains(const std::string& name) const;
   IOutputController* GetByName(const std::string& name) const;
   IOutputController* operator[](const std::string& name) const;
   static IOutputController* CreateController(const std::string& typeName);
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "OutputControllers"; }

private:
   void Clear();

   size_t m_initializedCount;
};

}
