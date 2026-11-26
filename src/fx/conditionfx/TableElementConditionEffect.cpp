#include "TableElementConditionEffect.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../table/TableElement.h"
#include "../../table/TableElementList.h"
#include "../../table/TableElementTypeEnum.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include <exprtk.hpp>
#include <stdexcept>
#include <algorithm>

namespace DOF
{

static bool ParseTableElementVariable(const std::string& variable, TableElementTypeEnum& elementType, int& number)
{
   if (variable.length() < 2)
      return false;

   switch (variable[0])
   {
   case 'L': elementType = TableElementTypeEnum::Lamp; break;
   case 'S': elementType = TableElementTypeEnum::Solenoid; break;
   case 'W': elementType = TableElementTypeEnum::Switch; break;
   case 'G': elementType = TableElementTypeEnum::GIString; break;
   case 'M': elementType = TableElementTypeEnum::Mech; break;
   case 'N': elementType = TableElementTypeEnum::GetMech; break;
   case 'E': elementType = TableElementTypeEnum::EMTable; break;
   case 'D': elementType = TableElementTypeEnum::LED; break;
   case 'C': elementType = TableElementTypeEnum::Score; break;
   case 'B': elementType = TableElementTypeEnum::ScoreDigit; break;
   default: return false;
   }

   return StringExtensions::TryParseInt(variable.substr(1), number);
}

TableElementConditionEffect::TableElementConditionEffect()
   : m_conditionExpressionValid(false)
   , m_table(nullptr)
{
   m_symbolTable = new exprtk::symbol_table<double>();
   m_expression = new exprtk::expression<double>();
   m_parser = new exprtk::parser<double>();
}

TableElementConditionEffect::~TableElementConditionEffect()
{
   delete m_symbolTable;
   delete m_expression;
   delete m_parser;
}

std::vector<std::string> TableElementConditionEffect::GetVariablesInternal()
{
   std::vector<std::string> variables;
   std::string c = m_condition;
   if (StringExtensions::IsNullOrWhiteSpace(c))
      return variables;

   if (!exprtk::collect_variables(StringExtensions::Replace(c, std::string(1, (char)TableElementTypeEnum::NamedElement), "NamedElement_"), variables))
   {
      Log::Warning(StringExtensions::Build("Cant extract variables from condition {0}.", m_condition));
      variables.clear();
   }

   for (std::string& variable : variables)
   {
      if (StringExtensions::StartsWith(variable, "namedelement_"))
         variable = "NamedElement_" + StringExtensions::ToUpper(variable.substr(13));
      else
         variable = StringExtensions::ToUpper(variable);
   }

   return variables;
}

std::vector<std::string> TableElementConditionEffect::GetVariables()
{
   std::vector<std::string> internalVars = GetVariablesInternal();
   std::vector<std::string> result;

   for (const std::string& var : internalVars)
   {
      if (StringExtensions::StartsWith(var, "NamedElement_"))
         result.push_back(std::string(1, (char)TableElementTypeEnum::NamedElement) + var.substr(13));
      else
         result.push_back(var);
   }

   return result;
}

void TableElementConditionEffect::InitCondition()
{
   m_conditionExpressionValid = false;

   if (StringExtensions::IsNullOrWhiteSpace(m_condition))
   {
      Log::Warning(StringExtensions::Build("No condition has been set for {0} named {1}.", "TableElementConditionEffect", NamedItemBase::GetName()));
      return;
   }

   std::vector<std::string> variables = GetVariablesInternal();

   m_variables.clear();
   m_symbolTable->clear();

   for (const std::string& var : variables)
   {
      TableElementTypeEnum elementType;
      int number;
      if (!StringExtensions::StartsWith(var, "NamedElement_") && !ParseTableElementVariable(var, elementType, number))
      {
         Log::Warning(StringExtensions::Build("The condition {0} of effect {1} refers to {2}, which is not a table element.", m_condition, NamedItemBase::GetName(), var));
         return;
      }
   }

   std::string transformedCondition = StringExtensions::Replace(m_condition, std::string(1, (char)TableElementTypeEnum::NamedElement), "NamedElement_");

   for (const std::string& var : variables)
   {
      m_variables[var] = 0.0;
      m_symbolTable->add_variable(var, m_variables[var]);
   }

   m_expression->register_symbol_table(*m_symbolTable);

   if (m_parser->compile(transformedCondition, *m_expression))
   {
      m_conditionExpressionValid = true;
   }
   else
   {
      m_conditionExpressionValid = false;
      Log::Exception(StringExtensions::Build(
         "A exception has occurred while compiling the condition {0} (internally translated to {1}) of effect {2}.", m_condition, transformedCondition, NamedItemBase::GetName()));
   }
}

void TableElementConditionEffect::UpdateVariableValues()
{
   if (!m_table || !m_table->GetTableElements())
      return;

   TableElementList* tableElements = m_table->GetTableElements();

   for (auto& varPair : m_variables)
   {
      const std::string& varName = varPair.first;
      double& varValue = varPair.second;

      if (StringExtensions::StartsWith(varName, "NamedElement_"))
      {
         std::string elementName = varName.substr(13);

         for (TableElement* element : *tableElements)
         {
            if (element->GetName() == elementName)
            {
               varValue = static_cast<double>(element->GetValue());
               break;
            }
         }
      }
      else
      {
         TableElementTypeEnum elementType;
         int number;
         if (!ParseTableElementVariable(varName, elementType, number))
            continue;

         for (TableElement* element : *tableElements)
         {
            if (element->GetTableElementType() == elementType && element->GetNumber() == number)
            {
               varValue = static_cast<double>(element->GetValue());
               break;
            }
         }
      }
   }
}

bool TableElementConditionEffect::EvaluateCondition()
{
   if (!m_conditionExpressionValid)
      return false;

   try
   {
      UpdateVariableValues();

      double result = m_expression->value();

      return result != 0.0;
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception during expression evaluation for {0}: {1}", m_condition, std::string(e.what())));
      return false;
   }
}

void TableElementConditionEffect::Trigger(TableElementData* tableElementData)
{
   if (m_conditionExpressionValid)
   {
      try
      {
         if (EvaluateCondition())
            tableElementData->m_value = 255;
         else
            tableElementData->m_value = 0;
         TriggerTargetEffect(tableElementData);
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("A exception occurred when evaluating the expression {0} of effect {1}. Effect will be deactivated.", m_condition, NamedItemBase::GetName()));
         m_conditionExpressionValid = false;
      }
   }
}

void TableElementConditionEffect::Init(Table* table)
{
   EffectEffectBase::Init(table);
   m_table = table;
   InitCondition();
}

void TableElementConditionEffect::Finish()
{
   m_conditionExpressionValid = false;
   m_table = nullptr;

   if (m_symbolTable)
      m_symbolTable->clear();
   m_variables.clear();

   EffectEffectBase::Finish();
}

}