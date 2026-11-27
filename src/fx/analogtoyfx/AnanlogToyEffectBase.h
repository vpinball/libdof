#pragma once

#include "../EffectBase.h"
#include "../../general/analog/AnalogAlpha.h"
#include <string>

namespace DOF
{

class Table;
class IAnalogAlphaToy;

class AnalogToyEffectBase : public EffectBase
{
public:
   AnalogToyEffectBase();
   virtual ~AnalogToyEffectBase() = default;

   const std::string& GetToyName() const { return m_toyName; }
   void SetToyName(const std::string& value);

   int GetLayerNr() const { return m_layerNr; }
   void SetLayerNr(int value) { m_layerNr = value; }

   virtual void Init(Table* table) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   Table* m_table;
   IAnalogAlphaToy* m_toy;
   AnalogAlpha* m_layer;

private:
   std::string m_toyName;
   int m_layerNr;

   void ResolveName(Table* table);
};

}