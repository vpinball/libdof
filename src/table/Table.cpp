#include "Table.h"

#include "../Log.h"
#include "../general/StringExtensions.h"
#include "../fx/AssignedEffectList.h"
#include "../fx/EffectList.h"
#include "TableElementList.h"
#include "TableElement.h"
#include "TableElementData.h"
#include "../Pinball.h"
#include "../globalconfiguration/GlobalConfig.h"
#include "../general/FileReader.h"
#include "../fx/matrixfx/bitmapshapes/ShapeDefinitions.h"
#include "../general/FilePattern.h"

#include <tinyxml2/tinyxml2.h>
#include <filesystem>
#include <stdexcept>
#include <fstream>


namespace DOF
{
Table::Table()
{
   m_pinball = nullptr;
   m_tableName = "";
   m_romName = "";
   m_tableFilename = "";
   m_tableConfigurationFilename = "";
   m_addLedControlConfig = false;
   m_configurationSource = TableConfigSourceEnum::Unknown;
   m_effects = new EffectList();
   m_tableElements = new TableElementList();
   m_assignedStaticEffects = new AssignedEffectList();
   m_shapeDefinitions = new ShapeDefinitions();
}

Table::~Table()
{
   delete m_effects;
   delete m_tableElements;
   delete m_assignedStaticEffects;
   delete m_shapeDefinitions;
   m_effects = nullptr;
   m_tableElements = nullptr;
   m_assignedStaticEffects = nullptr;
   m_shapeDefinitions = nullptr;
}

void Table::SetTableName(const std::string& tableName)
{
   if (tableName != m_tableName)
   {
      m_tableName = tableName;
   }
}

void Table::SetRomName(const std::string& romName)
{
   if (romName != m_romName)
   {
      m_romName = romName;
   }
}

void Table::UpdateTableElement(TableElementData* data) { m_tableElements->UpdateState(data); }

void Table::UpdateTableElement(const std::string& elementName, int value)
{
   TableElementData data(TableElementTypeEnum::NamedElement, 0, value);
   data.m_name = elementName;
   UpdateTableElement(&data);
}

void Table::UpdateTableElement(TableElementTypeEnum elementType, int number, int value)
{
   TableElementData data(elementType, number, value);
   UpdateTableElement(&data);
}

void Table::TriggerStaticEffects()
{
   TableElementData data(TableElementTypeEnum::Unknown, 0, 1);
   if (m_assignedStaticEffects)
      m_assignedStaticEffects->Trigger(data);
}

void Table::Init(Pinball* pinball)
{
   m_pinball = pinball;

   FileInfo* shapeDefinitionFile = pinball->GetGlobalConfig()->GetShapeDefinitionFile();
   if (shapeDefinitionFile && shapeDefinitionFile->Exists())
   {
      Log::Write(StringExtensions::Build("Loading shape definition file: {0}", shapeDefinitionFile->FullName()));
      try
      {
         ShapeDefinitions* shapeDefs = ShapeDefinitions::GetShapeDefinitionsFromShapeDefinitionsXmlFile(shapeDefinitionFile->FullName());
         if (shapeDefs)
         {
            delete m_shapeDefinitions;
            m_shapeDefinitions = shapeDefs;

            std::string pngPath = shapeDefinitionFile->FullName();
            size_t lastDot = pngPath.find_last_of('.');
            if (lastDot != std::string::npos)
            {
               pngPath = pngPath.substr(0, lastDot) + ".png";
            }
            m_shapeDefinitions->SetBitmapFilePattern(new FilePattern(pngPath));
         }
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("Loading shape definition file {0} failed.", shapeDefinitionFile->FullName()));
      }
   }
   else
   {
      if (shapeDefinitionFile == nullptr)
      {
         Log::Warning("Could not determine name of shape definition file");
      }
      else
      {
         Log::Warning(StringExtensions::Build("Shape definition file {0} does not exist", shapeDefinitionFile->FullName()));
      }
   }

   delete shapeDefinitionFile;

   if (m_effects)
      m_effects->Init(this);
   if (m_tableElements)
      m_tableElements->Init(this);
   if (m_assignedStaticEffects)
      m_assignedStaticEffects->Init(this);
}

std::string Table::ToXml() const
{
   tinyxml2::XMLDocument doc;
   doc.InsertEndChild(doc.NewDeclaration());

   tinyxml2::XMLElement* root = doc.NewElement("Table");
   doc.InsertEndChild(root);

   if (!m_tableName.empty())
   {
      tinyxml2::XMLElement* element = doc.NewElement("TableName");
      element->SetText(m_tableName.c_str());
      root->InsertEndChild(element);
   }

   tinyxml2::XMLElement* element = doc.NewElement("AddLedControlConfig");
   element->SetText(m_addLedControlConfig);
   root->InsertEndChild(element);

   if (m_effects)
   {
      tinyxml2::XMLElement* effectsElement = m_effects->ToXml(doc);
      if (effectsElement)
         root->InsertEndChild(effectsElement);
   }

   if (m_tableElements && !m_tableElements->empty())
   {
      tinyxml2::XMLElement* tableElementsElement = doc.NewElement("TableElements");
      for (const auto& tableElement : *m_tableElements)
      {
         if (tableElement)
         {
            tinyxml2::XMLElement* elementNode = doc.NewElement("TableElement");

            tinyxml2::XMLElement* typeElement = doc.NewElement("TableElementType");
            typeElement->SetText((int)tableElement->GetTableElementType());
            elementNode->InsertEndChild(typeElement);

            tinyxml2::XMLElement* numberElement = doc.NewElement("Number");
            numberElement->SetText(tableElement->GetNumber());
            elementNode->InsertEndChild(numberElement);

            if (!tableElement->GetName().empty())
            {
               tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
               nameElement->SetText(tableElement->GetName().c_str());
               elementNode->InsertEndChild(nameElement);
            }

            tableElementsElement->InsertEndChild(elementNode);
         }
      }
      root->InsertEndChild(tableElementsElement);
   }

   if (m_assignedStaticEffects)
   {
      tinyxml2::XMLElement* staticEffectsElement = doc.NewElement("AssignedStaticEffects");

      root->InsertEndChild(staticEffectsElement);
   }

   tinyxml2::XMLPrinter printer;
   doc.Print(&printer);
   return std::string(printer.CStr());
}

void Table::SaveConfigXmlFile(const std::string& filename)
{
   try
   {
      std::string xml = ToXml();
      std::ofstream file(filename);
      if (file.is_open())
      {
         file << xml;
         file.close();
         Log::Write(StringExtensions::Build("Table configuration saved to: {0}", filename));
      }
      else
      {
         Log::Warning(StringExtensions::Build("Failed to open file for writing: {0}", filename));
      }
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception saving table config to file {0}: {1}", filename, e.what()));
   }
}

Table* Table::GetTableFromConfigXmlFile(const std::string& filename)
{
   try
   {
      if (std::filesystem::exists(filename))
      {
         std::string xml = FileReader::ReadFileToString(filename);
         Table* table = FromXml(xml);
         if (table)
         {
            table->SetConfigurationSource(TableConfigSourceEnum::TableConfigurationFile);
            Log::Write(StringExtensions::Build("Table configuration loaded from: {0}", filename));
         }
         return table;
      }
      else
      {
         Log::Warning(StringExtensions::Build("Table configuration file does not exist: {0}", filename));
         return nullptr;
      }
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception loading table config from file {0}: {1}", filename, e.what()));
      return nullptr;
   }
}

Table* Table::FromXml(const std::string& configXml)
{
   tinyxml2::XMLDocument doc;
   if (doc.Parse(configXml.c_str()) != tinyxml2::XML_SUCCESS)
   {
      Log::Warning(StringExtensions::Build("Table XML parse error: {0}", doc.ErrorStr()));
      return nullptr;
   }

   tinyxml2::XMLElement* root = doc.FirstChildElement("Table");
   if (!root)
   {
      Log::Warning("Table root element not found");
      return nullptr;
   }

   Table* table = new Table();
   try
   {
      tinyxml2::XMLElement* element = root->FirstChildElement("TableName");
      if (element && element->GetText())
         table->SetTableName(element->GetText());

      element = root->FirstChildElement("AddLedControlConfig");
      if (element)
      {
         bool value;
         if (element->QueryBoolText(&value) == tinyxml2::XML_SUCCESS)
            table->SetAddLedControlConfig(value);
      }

      element = root->FirstChildElement("Effects");
      if (element)
      {
         table->GetEffects()->FromXml(element);
      }

      element = root->FirstChildElement("TableElements");
      if (element)
      {
         tinyxml2::XMLElement* tableElementNode = element->FirstChildElement("TableElement");
         while (tableElementNode)
         {
            tinyxml2::XMLElement* typeElement = tableElementNode->FirstChildElement("TableElementType");
            TableElementTypeEnum type = TableElementTypeEnum::Lamp;
            std::string typeName = "Lamp";
            if (typeElement)
            {
               int value;
               typeName = typeElement->GetText() ? typeElement->GetText() : "";
               if (StringExtensions::TryParseInt(typeName, value))
               {
                  if (value < 0 || value > 127 || std::string("?LWSGMNEDCB$").find((char)value) == std::string::npos)
                     throw std::runtime_error(StringExtensions::Build("{0} is not a valid value for TableElementType", typeName));
                  type = (TableElementTypeEnum)value;
               }
               else if (typeName == "Unknown")
                  type = TableElementTypeEnum::Unknown;
               else if (typeName == "Lamp")
                  type = TableElementTypeEnum::Lamp;
               else if (typeName == "Switch")
                  type = TableElementTypeEnum::Switch;
               else if (typeName == "Solenoid")
                  type = TableElementTypeEnum::Solenoid;
               else if (typeName == "GIString")
                  type = TableElementTypeEnum::GIString;
               else if (typeName == "Mech")
                  type = TableElementTypeEnum::Mech;
               else if (typeName == "GetMech")
                  type = TableElementTypeEnum::GetMech;
               else if (typeName == "EMTable")
                  type = TableElementTypeEnum::EMTable;
               else if (typeName == "LED")
                  type = TableElementTypeEnum::LED;
               else if (typeName == "Score")
                  type = TableElementTypeEnum::Score;
               else if (typeName == "ScoreDigit")
                  type = TableElementTypeEnum::ScoreDigit;
               else if (typeName == "NamedElement")
                  type = TableElementTypeEnum::NamedElement;
               else
                  throw std::runtime_error(StringExtensions::Build("{0} is not a valid value for TableElementType", typeName));
            }

            tinyxml2::XMLElement* numberElement = tableElementNode->FirstChildElement("Number");
            int number = 0;
            if (numberElement)
            {
               std::string numberText = numberElement->GetText() ? numberElement->GetText() : "";
               if (!StringExtensions::TryParseInt(numberText, number))
                  throw std::runtime_error(StringExtensions::Build("{0} is not a valid value for Number", numberText));
            }

            tinyxml2::XMLElement* nameElement = tableElementNode->FirstChildElement("Name");
            std::string name = "";
            if (nameElement && nameElement->GetText())
               name = StringExtensions::ToUpper(nameElement->GetText());

            if (type == TableElementTypeEnum::NamedElement && StringExtensions::IsNullOrWhiteSpace(name))
               throw std::runtime_error("Named TableElements cant have a empty name when they are added to the list.");

            for (TableElement* existing : *table->GetTableElements())
            {
               if (type != TableElementTypeEnum::NamedElement && existing->GetTableElementType() == type && existing->GetNumber() == number)
                  throw std::runtime_error(StringExtensions::Build(
                     "The TableElement {0} {1} cant be added to the list. Another entry with the same type and number does already exist.", typeName, std::to_string(number)));
               if (type == TableElementTypeEnum::NamedElement && existing->GetTableElementType() == TableElementTypeEnum::NamedElement && existing->GetName() == name)
                  throw std::runtime_error(
                     StringExtensions::Build("The TableElement named {0} cant be added to the list. Another entry with the same name does already exist.", name));
            }

            TableElement* tableElement = new TableElement();
            tableElement->SetTableElementType(type);
            tableElement->SetNumber(number);
            tableElement->SetName(name);
            table->GetTableElements()->push_back(tableElement);

            tinyxml2::XMLElement* assignedEffectsElement = tableElementNode->FirstChildElement("AssignedEffects");
            if (assignedEffectsElement)
               tableElement->GetAssignedEffects()->FromXml(assignedEffectsElement);

            tableElementNode = tableElementNode->NextSiblingElement("TableElement");
         }
      }

      element = root->FirstChildElement("AssignedStaticEffects");
      if (element)
      {
         table->GetAssignedStaticEffects()->FromXml(element);
      }
   }
   catch (...)
   {
      delete table;
      throw;
   }

   table->SetConfigurationSource(TableConfigSourceEnum::TableConfigurationFile);
   Log::Write("Table configuration loaded from XML");
   return table;
}

void Table::Finish()
{
   if (m_assignedStaticEffects)
      m_assignedStaticEffects->Finish();
   if (m_tableElements)
      m_tableElements->FinishAssignedEffects();
   if (m_effects)
      m_effects->Finish();
   m_pinball = nullptr;
}

std::string Table::GetConfigXml() const { return ToXml(); }

Table* Table::GetTableFromConfigXml(const std::string& configXml) { return FromXml(configXml); }

}