#pragma once

#include "../EffectBase.h"
#include "../../general/color/RGBAColor.h"
#include <string>

namespace DOF
{

class Table;
class IRGBAToy;

class RGBAEffectBase : public EffectBase
{
public:
   RGBAEffectBase();
   virtual ~RGBAEffectBase() = default;

   const std::string& GetToyName() const { return m_toyName; }
   void SetToyName(const std::string& value);
   int GetLayerNr() const { return m_layerNr; }
   void SetLayerNr(int value) { m_layerNr = value; }
   virtual void Init(Table* table) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   Table* m_table;
   IRGBAToy* m_rgbaToy;
   RGBAColor* m_layer;

private:
   std::string m_toyName;
   int m_layerNr;
};

}