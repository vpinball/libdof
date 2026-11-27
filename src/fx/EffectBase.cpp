#include "EffectBase.h"
#include "../Log.h"
#include "../general/color/RGBAColor.h"
#include "../general/analog/AnalogAlpha.h"
#include "../general/StringExtensions.h"

#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace DOF
{

EffectBase::EffectBase() { }

void EffectBase::Finish() { }

tinyxml2::XMLElement* EffectBase::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
   {
      tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
      nameElement->SetText(GetName().c_str());
      element->InsertEndChild(nameElement);
   }

   SerializeToXml(element, doc);

   return element;
}

bool EffectBase::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const tinyxml2::XMLElement* nameElement = element->FirstChildElement("Name");
   if (nameElement && nameElement->GetText())
   {
      SetName(nameElement->GetText());
   }

   DeserializeFromXml(element);

   return true;
}

void EffectBase::SerializeToXml(tinyxml2::XMLElement* element, tinyxml2::XMLDocument& doc) const { }

void EffectBase::DeserializeFromXml(const tinyxml2::XMLElement* element) { }

void EffectBase::ThrowInvalidXmlValue(const char* name, const std::string& text)
{
   throw std::runtime_error(StringExtensions::Build("{0} is not a valid value for {1}", text, name));
}

bool EffectBase::ReadXmlString(const tinyxml2::XMLElement* element, const char* name, std::string& value)
{
   const tinyxml2::XMLElement* child = element->FirstChildElement(name);
   if (!child)
      return false;
   value = child->GetText() ? child->GetText() : "";
   return true;
}

bool EffectBase::ReadXmlInt(const tinyxml2::XMLElement* element, const char* name, int& value)
{
   std::string text;
   if (!ReadXmlString(element, name, text))
      return false;
   int number;
   if (!StringExtensions::TryParseInt(text, number))
      ThrowInvalidXmlValue(name, text);
   value = number;
   return true;
}

bool EffectBase::ReadXmlFloat(const tinyxml2::XMLElement* element, const char* name, float& value)
{
   std::string text;
   if (!ReadXmlString(element, name, text))
      return false;
   std::string trimmed = StringExtensions::Trim(text);
   float number;
   if (trimmed == "INF")
      number = std::numeric_limits<float>::infinity();
   else if (trimmed == "-INF")
      number = -std::numeric_limits<float>::infinity();
   else if (trimmed == "NaN")
      number = std::numeric_limits<float>::quiet_NaN();
   else
   {
      std::istringstream stream(trimmed);
      stream.imbue(std::locale::classic());
      if (!(stream >> number) || !stream.eof())
         ThrowInvalidXmlValue(name, text);
   }
   value = number;
   return true;
}

bool EffectBase::ReadXmlRGBAColor(const tinyxml2::XMLElement* element, const char* name, RGBAColor& value)
{
   const tinyxml2::XMLElement* child = element->FirstChildElement(name);
   if (!child)
      return false;
   RGBAColor color;
   std::string hexColor;
   if (ReadXmlString(child, "HexColor", hexColor))
      color.SetHexColor(hexColor);
   value = color;
   return true;
}

bool EffectBase::ReadXmlAnalogAlpha(const tinyxml2::XMLElement* element, const char* name, AnalogAlpha& value)
{
   const tinyxml2::XMLElement* child = element->FirstChildElement(name);
   if (!child)
      return false;
   AnalogAlpha analogAlpha;
   int number;
   if (ReadXmlInt(child, "Value", number))
      analogAlpha.SetValue(number);
   if (ReadXmlInt(child, "Alpha", number))
      analogAlpha.SetAlpha(number);
   value = analogAlpha;
   return true;
}

bool EffectBase::ReadXmlFadeMode(const tinyxml2::XMLElement* element, const char* name, FadeModeEnum& value)
{
   std::string text;
   if (!ReadXmlString(element, name, text))
      return false;
   if (text == "Fade")
      value = FadeModeEnum::Fade;
   else if (text == "OnOff")
      value = FadeModeEnum::OnOff;
   else
      ThrowInvalidXmlValue(name, text);
   return true;
}

bool EffectBase::ReadXmlRetriggerBehaviour(const tinyxml2::XMLElement* element, const char* name, RetriggerBehaviourEnum& value)
{
   std::string text;
   if (!ReadXmlString(element, name, text))
      return false;
   if (text == "Restart")
      value = RetriggerBehaviourEnum::Restart;
   else if (text == "Ignore")
      value = RetriggerBehaviourEnum::Ignore;
   else
      ThrowInvalidXmlValue(name, text);
   return true;
}

}