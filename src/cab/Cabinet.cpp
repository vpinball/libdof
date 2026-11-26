#include "Cabinet.h"

#include "../Log.h"
#include "../general/StringExtensions.h"
#include "../general/FileInfo.h"
#include "../general/FileReader.h"
#include "../general/CurveList.h"
#include "../general/color/ColorList.h"
#include "CabinetOutputList.h"
#include "out/IAutoConfigOutputController.h"
#include "out/OutputControllerList.h"
#include "toys/ToyList.h"
#include "toys/lwequivalent/LedWizEquivalent.h"
#include "toys/hardware/LedStrip.h"
#include "schedules/ScheduledSettings.h"
#include "sequencer/SequentialOutputSettings.h"
#include "overrides/TableOverrideSettings.h"
#include <sstream>
#include <exception>

#ifdef __HIDAPI__
#include "out/ps/Pinscape.h"
#include "out/ps/PinscapeAutoConfigurator.h"
#include "out/pspico/PinscapePico.h"
#include "out/pspico/PinscapePicoAutoConfigurator.h"
#include "out/lw/LedWizAutoConfigurator.h"
#include "out/dudescab/DudesCabAutoConfigurator.h"
#include "out/adressableledstrip/UMXControllerAutoConfigurator.h"
#endif

#ifdef __LIBUSB__
#include "out/pac/PacLed64AutoConfigurator.h"
#include "out/pac/PacDriveAutoConfigurator.h"
#include "out/pac/PacUIOAutoConfigurator.h"
#endif

#ifdef __LIBFTDI__
#include "out/ftdichip/FT245RBitbangControllerAutoConfigurator.h"
#endif

#ifdef __LIBSERIALPORT__
#include "out/pinone/PinOneAutoConfigurator.h"
#endif

namespace DOF
{

Cabinet::Cabinet()
{
   m_autoConfigEnabled = true;
   m_outputControllers = new OutputControllerList();
   m_outputs = new CabinetOutputList(this);
   m_toys = new ToyList();
   m_curves = new CurveList();
   m_colors = new ColorList();

#ifdef __HIDAPI__
   Pinscape::Initialize();
   PinscapePico::Initialize();
#endif
}

Cabinet::~Cabinet()
{
   if (m_colors)
   {
      delete m_colors;
      m_colors = nullptr;
   }
   if (m_curves)
   {
      delete m_curves;
      m_curves = nullptr;
   }
   if (m_toys)
   {
      delete m_toys;
      m_toys = nullptr;
   }
   if (m_outputs)
   {
      m_outputs->SetCabinet(nullptr); // Break cycle before deletion
      delete m_outputs;
      m_outputs = nullptr;
   }
   if (m_outputControllers)
   {
      delete m_outputControllers;
      m_outputControllers = nullptr;
   }
}

void Cabinet::AutoConfig()
{
   Log::Write("Cabinet auto configuration started");

   std::vector<IAutoConfigOutputController*> items;

#ifdef __HIDAPI__
   items.push_back(new PinscapeAutoConfigurator());
   items.push_back(new PinscapePicoAutoConfigurator());
   items.push_back(new LedWizAutoConfigurator());
   items.push_back(new DudesCabAutoConfigurator());
   items.push_back(new UMXControllerAutoConfigurator());
#endif

#ifdef __LIBUSB__
   items.push_back(new PacLed64AutoConfigurator());
   items.push_back(new PacDriveAutoConfigurator());
   items.push_back(new PacUIOAutoConfigurator());
#endif

#ifdef __LIBFTDI__
   items.push_back(new FT245RBitbangControllerAutoConfigurator());
#endif

#ifdef __LIBSERIALPORT__
   items.push_back(new PinOneAutoConfigurator());
#endif

   for (auto& item : items)
      item->AutoConfig(this);

   for (auto& item : items)
      delete item;

   Log::Write("Cabinet auto configuration finished");
}

ScheduledSettings* Cabinet::GetScheduledSettings() { return &ScheduledSettings::GetInstance(); }

SequentialOutputSettings* Cabinet::GetSequentialOutputSettings() { return &SequentialOutputSettings::GetInstance(); }

TableOverrideSettings* Cabinet::GetTableOverrideSettings() { return TableOverrideSettings::GetInstance(); }

void Cabinet::SetTableOverrideSettings(TableOverrideSettings* tableOverrideSettings) { }

std::string Cabinet::GetConfigXml()
{
   tinyxml2::XMLDocument doc;
   tinyxml2::XMLElement* root = ToXml(doc);
   if (!root)
      return "";

   doc.InsertFirstChild(root);

   tinyxml2::XMLPrinter printer;
   doc.Print(&printer);
   return printer.CStr();
}

void Cabinet::SaveConfigXmlFile(const std::string& filename)
{
   std::string xml = GetConfigXml();
   if (xml.empty())
   {
      Log::Warning("Cannot save cabinet config - XML generation failed");
      return;
   }

   tinyxml2::XMLDocument doc;
   if (doc.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS)
   {
      Log::Warning("Cannot save cabinet config - XML parsing failed");
      return;
   }

   if (doc.SaveFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
   {
      Log::Warning(StringExtensions::Build("Failed to save cabinet config to file: {0}", filename));
   }
   else
   {
      Log::Write(StringExtensions::Build("Cabinet config saved to: {0}", filename));
   }
}

Cabinet* Cabinet::GetCabinetFromConfigXmlFile(FileInfo* cabinetConfigFile) { return cabinetConfigFile ? GetCabinetFromConfigXmlFile(cabinetConfigFile->FullName()) : nullptr; }

Cabinet* Cabinet::GetCabinetFromConfigXmlFile(const std::string& filename)
{
   std::string xml;

   try
   {
      xml = FileReader::ReadFileToString(filename);
   }
   catch (...)
   {
      Log::Exception(StringExtensions::Build("Could not load cabinet config from {0}.", filename));
   }

   return GetCabinetFromConfigXml(xml);
}

Cabinet* Cabinet::GetCabinetFromConfigXml(const std::string& configXml)
{
   if (configXml.empty())
   {
      Log::Warning("Cannot load cabinet config from empty XML");
      return nullptr;
   }

   tinyxml2::XMLDocument doc;
   if (doc.Parse(configXml.c_str()) != tinyxml2::XML_SUCCESS)
   {
      Log::Exception(StringExtensions::Build("Could not parse cabinet config XML: {0}", doc.ErrorStr()));
      return nullptr;
   }

   tinyxml2::XMLElement* root = doc.FirstChildElement("Cabinet");
   if (!root)
   {
      Log::Exception("Cabinet config XML missing root Cabinet element");
      return nullptr;
   }

   Cabinet* cabinet = new Cabinet();
   bool loaded;
   try
   {
      loaded = cabinet->FromXml(root);
   }
   catch (...)
   {
      delete cabinet;
      throw;
   }
   if (!loaded)
   {
      delete cabinet;
      Log::Exception("Failed to deserialize cabinet config from XML");
      return nullptr;
   }

   return cabinet;
}

bool Cabinet::TestCabinetConfigXmlFile(const std::string& filename)
{
   try
   {
      std::string xml = FileReader::ReadFileToString(filename);
      Cabinet* testCabinet = GetCabinetFromConfigXml(xml);
      if (testCabinet)
      {
         delete testCabinet;
         return true;
      }
   }
   catch (...)
   {
      Log::Exception(StringExtensions::Build("Error testing cabinet config file: {0}", filename));
   }

   return false;
}

void Cabinet::Init(ICabinetOwner* cabinetOwner)
{
   Log::Write("Initializing cabinet");
   m_owner = cabinetOwner;
   m_outputControllers->Init(this);
   m_toys->Init(this);

   m_outputs->ConnectOutputsToControllers();

   Log::Write("Cabinet initialized");
}

void Cabinet::Update()
{
   m_toys->UpdateOutputs();
   m_outputControllers->Update();
}

void Cabinet::Finish()
{
   Log::Write("Finishing cabinet");

   std::exception_ptr toyError;
   std::string toyMessage;
   try
   {
      m_toys->Finish();
   }
   catch (const std::exception& e)
   {
      toyError = std::current_exception();
      toyMessage = e.what();
   }
   catch (...)
   {
      toyError = std::current_exception();
      toyMessage = "unknown exception";
   }
   std::exception_ptr controllerError;
   std::string controllerMessage;
   try
   {
      m_outputControllers->Finish();
   }
   catch (const std::exception& e)
   {
      controllerError = std::current_exception();
      controllerMessage = e.what();
   }
   catch (...)
   {
      controllerError = std::current_exception();
      controllerMessage = "unknown exception";
   }
   if (toyError && controllerError)
      throw std::runtime_error(StringExtensions::Build("Finishing the toys failed: {0}. Finishing the output controllers failed: {1}", toyMessage, controllerMessage));
   if (toyError)
      std::rethrow_exception(toyError);
   if (controllerError)
      std::rethrow_exception(controllerError);
   Log::Write("Cabinet finished");
}

tinyxml2::XMLElement* Cabinet::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!m_name.empty())
      element->SetAttribute("Name", m_name.c_str());

   tinyxml2::XMLElement* autoConfigElement = doc.NewElement("AutoConfigEnabled");
   autoConfigElement->SetText(m_autoConfigEnabled ? "true" : "false");
   element->InsertEndChild(autoConfigElement);

   if (m_outputControllers)
   {
      tinyxml2::XMLElement* controllersElement = m_outputControllers->ToXml(doc);
      if (controllersElement)
         element->InsertEndChild(controllersElement);
   }

   if (m_toys)
   {
      tinyxml2::XMLElement* toysElement = doc.NewElement("Toys");
      element->InsertEndChild(toysElement);
   }

   if (m_colors && !m_colors->Empty())
   {
      tinyxml2::XMLElement* colorsElement = m_colors->ToXml(doc);
      if (colorsElement)
         element->InsertEndChild(colorsElement);
   }

   if (m_curves && !m_curves->empty())
   {
      tinyxml2::XMLElement* curvesElement = m_curves->ToXml(doc);
      if (curvesElement)
         element->InsertEndChild(curvesElement);
   }

   ScheduledSettings* scheduledSettings = &ScheduledSettings::GetInstance();
   if (scheduledSettings && !scheduledSettings->empty())
   {
      tinyxml2::XMLElement* scheduledElement = scheduledSettings->ToXml(doc);
      if (scheduledElement)
         element->InsertEndChild(scheduledElement);
   }

   SequentialOutputSettings* sequentialSettings = &SequentialOutputSettings::GetInstance();
   if (sequentialSettings && !sequentialSettings->empty())
   {
      tinyxml2::XMLElement* sequentialElement = sequentialSettings->ToXml(doc);
      if (sequentialElement)
         element->InsertEndChild(sequentialElement);
   }

   TableOverrideSettings* tableOverrideSettings = TableOverrideSettings::GetInstance();
   if (tableOverrideSettings)
   {
   }

   return element;
}

bool Cabinet::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const char* name = element->Attribute("Name");
   if (name)
      m_name = name;

   const tinyxml2::XMLElement* autoConfigElement = element->FirstChildElement("AutoConfigEnabled");
   if (autoConfigElement && autoConfigElement->GetText())
   {
      std::string autoConfigText = StringExtensions::ToLower(autoConfigElement->GetText());
      m_autoConfigEnabled = (autoConfigText == "true");
   }

   const tinyxml2::XMLElement* controllersElement = element->FirstChildElement("OutputControllers");
   if (controllersElement && m_outputControllers)
   {
      m_outputControllers->FromXml(controllersElement);
   }

   const tinyxml2::XMLElement* colorsElement = element->FirstChildElement("ColorList");
   if (colorsElement && m_colors)
   {
      m_colors->FromXml(colorsElement);
   }

   const tinyxml2::XMLElement* curvesElement = element->FirstChildElement("CurveList");
   if (curvesElement && m_curves)
   {
      m_curves->FromXml(curvesElement);
   }

   const tinyxml2::XMLElement* tableOverrideElement = element->FirstChildElement("TableOverrideSettings");
   if (tableOverrideElement)
   {
   }

   const tinyxml2::XMLElement* toysElement = element->FirstChildElement("Toys");
   if (toysElement && m_toys)
   {
      for (const tinyxml2::XMLElement* toyElement = toysElement->FirstChildElement(); toyElement; toyElement = toyElement->NextSiblingElement())
      {
         const char* toyType = toyElement->Name();
         if (toyType)
         {
            if (std::string(toyType) == "LedWizEquivalent")
            {
               LedWizEquivalent* ledWizEquivalent = new LedWizEquivalent();
               if (ledWizEquivalent->FromXml(toyElement))
               {
                  m_toys->push_back(ledWizEquivalent);
               }
               else
               {
                  delete ledWizEquivalent;
               }
            }
            else if (std::string(toyType) == "LedStrip")
            {
               LedStrip* ledStrip = new LedStrip();
               if (ledStrip->FromXml(toyElement))
               {
                  m_toys->push_back(ledStrip);
               }
               else
               {
                  delete ledStrip;
               }
            }
         }
      }
   }

   const tinyxml2::XMLElement* scheduledElement = element->FirstChildElement("ScheduledSettings");
   if (scheduledElement)
   {
      ScheduledSettings& scheduledSettings = ScheduledSettings::GetInstance();
      scheduledSettings.FromXml(scheduledElement);
   }

   const tinyxml2::XMLElement* sequentialElement = element->FirstChildElement("SequentialOutputSettings");
   if (sequentialElement)
   {
      SequentialOutputSettings& sequentialSettings = SequentialOutputSettings::GetInstance();
      sequentialSettings.FromXml(sequentialElement);
   }

   return true;
}

}