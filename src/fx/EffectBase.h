#pragma once

#include "../general/generic/NamedItemBase.h"
#include "IEffect.h"
#include "FadeModeEnum.h"
#include "RetriggerBehaviourEnum.h"

namespace DOF
{

class Table;
class TableElementData;
class RGBAColor;
class AnalogAlpha;

class EffectBase : public NamedItemBase, public IEffect
{
public:
   EffectBase();
   virtual ~EffectBase() = default;


   const std::string& GetName() const override { return NamedItemBase::GetName(); }
   void SetName(const std::string& name) override { NamedItemBase::SetName(name); }


   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override = 0;

   virtual void Trigger(TableElementData* tableElementData) override = 0;
   virtual void Init(Table* table) override = 0;
   virtual void Finish() override;

protected:
   virtual void SerializeToXml(tinyxml2::XMLElement* element, tinyxml2::XMLDocument& doc) const;
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element);

   static void ThrowInvalidXmlValue(const char* name, const std::string& text);
   static bool ReadXmlString(const tinyxml2::XMLElement* element, const char* name, std::string& value);
   static bool ReadXmlInt(const tinyxml2::XMLElement* element, const char* name, int& value);
   static bool ReadXmlFloat(const tinyxml2::XMLElement* element, const char* name, float& value);
   static bool ReadXmlRGBAColor(const tinyxml2::XMLElement* element, const char* name, RGBAColor& value);
   static bool ReadXmlAnalogAlpha(const tinyxml2::XMLElement* element, const char* name, AnalogAlpha& value);
   static bool ReadXmlFadeMode(const tinyxml2::XMLElement* element, const char* name, FadeModeEnum& value);
   static bool ReadXmlRetriggerBehaviour(const tinyxml2::XMLElement* element, const char* name, RetriggerBehaviourEnum& value);
};

}