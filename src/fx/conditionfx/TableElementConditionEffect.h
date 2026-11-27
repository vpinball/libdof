#pragma once

#include "../EffectEffectBase.h"
#include <string>
#include <vector>
#include <unordered_map>


namespace exprtk
{
template <typename T> class symbol_table;
template <typename T> class expression;
template <typename T> class parser;
}

namespace DOF
{

class TableElementData;

class TableElementConditionEffect : public EffectEffectBase
{
public:
   TableElementConditionEffect();
   virtual ~TableElementConditionEffect();

   const std::string& GetCondition() const { return m_condition; }
   void SetCondition(const std::string& value) { m_condition = value; }
   std::vector<std::string> GetVariables();
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;


   virtual std::string GetXmlElementName() const override { return "TableElementConditionEffect"; }

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

private:
   std::string m_condition;
   bool m_conditionExpressionValid;
   exprtk::symbol_table<double>* m_symbolTable;
   exprtk::expression<double>* m_expression;
   exprtk::parser<double>* m_parser;
   std::unordered_map<std::string, double> m_variables;
   Table* m_table;

   std::vector<std::string> GetVariablesInternal();
   void InitCondition();
   bool EvaluateCondition();
   void UpdateVariableValues();
};

}