#include "LedWizEquivalent.h"
#include "../../Cabinet.h"
#include "../../CabinetOutputList.h"
#include "../ToyList.h"
#include "../../out/OutputControllerList.h"
#include "../../out/IOutputController.h"
#include "../../out/OutputControllerBase.h"
#include "../../out/OutputList.h"
#include "../../out/Output.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <tinyxml2/tinyxml2.h>

#include <algorithm>

namespace DOF
{

LedWizEquivalentOutput::LedWizEquivalentOutput()
   : m_ledWizEquivalentOutputNumber(0)
   , m_output(nullptr)
{
}

LedWizEquivalentOutput::LedWizEquivalentOutput(int outputNumber, const std::string& outputName)
   : m_outputName(outputName)
   , m_ledWizEquivalentOutputNumber(outputNumber)
   , m_output(nullptr)
{
}

LedWizEquivalentOutput::~LedWizEquivalentOutput() { }

LedWizEquivalentOutputList::LedWizEquivalentOutputList() { }

LedWizEquivalentOutputList::~LedWizEquivalentOutputList()
{
   for (LedWizEquivalentOutput* output : *this)
   {
      delete output;
   }
   clear();
}

LedWizEquivalentOutput* LedWizEquivalentOutputList::FindByNumber(int number) const
{
   for (LedWizEquivalentOutput* output : *this)
   {
      if (output != nullptr && output->GetLedWizEquivalentOutputNumber() == number)
      {
         return output;
      }
   }
   return nullptr;
}

LedWizEquivalentOutput* LedWizEquivalentOutputList::FindByName(const std::string& name) const
{
   for (LedWizEquivalentOutput* output : *this)
   {
      if (output != nullptr && output->GetOutputName() == name)
      {
         return output;
      }
   }
   return nullptr;
}

void LedWizEquivalentOutputList::AddOutput(LedWizEquivalentOutput* output)
{
   if (output == nullptr)
      return;

   if (FindByNumber(output->GetLedWizEquivalentOutputNumber()) != nullptr)
   {
      Log::Warning(StringExtensions::Build("LedWizEquivalentOutput with number {0} already exists", std::to_string(output->GetLedWizEquivalentOutputNumber())));
      return;
   }

   push_back(output);
}

LedWizEquivalentOutput* LedWizEquivalentOutputList::GetOrCreateOutput(int number)
{
   LedWizEquivalentOutput* output = FindByNumber(number);
   if (output == nullptr)
   {
      output = new LedWizEquivalentOutput(number, "");
      AddOutput(output);
   }
   return output;
}

LedWizEquivalent::LedWizEquivalent()
   : m_ledWizNumber(-1)
{
}

LedWizEquivalent::LedWizEquivalent(int ledWizNumber)
   : m_ledWizNumber(ledWizNumber)
{
}

LedWizEquivalent::~LedWizEquivalent() { }

void LedWizEquivalent::Init(Cabinet* cabinet)
{
   if (cabinet == nullptr)
   {
      Log::Warning("LedWizEquivalent::Init - Cabinet is null");
      return;
   }

   Log::Write(StringExtensions::Build("Initializing LedWizEquivalent: {0} (LedWiz {1})", GetName(), std::to_string(m_ledWizNumber)));

   ResolveOutputs(cabinet);

   Log::Write(StringExtensions::Build("LedWizEquivalent {0} initialized with {1} outputs", std::to_string(m_ledWizNumber), std::to_string(static_cast<int>(m_outputs.size()))));
}

void LedWizEquivalent::Reset()
{
   for (LedWizEquivalentOutput* output : m_outputs)
   {
      if (output != nullptr && output->GetOutput() != nullptr)
         output->GetOutput()->SetOutput(0);
   }
}

void LedWizEquivalent::Finish()
{
   Log::Write(StringExtensions::Build("Finishing LedWizEquivalent: {0} (LedWiz {1})", GetName(), std::to_string(m_ledWizNumber)));
   Reset();
}

IOutput* LedWizEquivalent::GetOutput(int outputNumber)
{
   LedWizEquivalentOutput* ledWizOutput = m_outputs.FindByNumber(outputNumber);
   if (ledWizOutput != nullptr)
      return ledWizOutput->GetOutput();
   return nullptr;
}

void LedWizEquivalent::SetupOutputs(int numberOfOutputs)
{
   Log::Write(StringExtensions::Build("Setting up {0} outputs for LedWizEquivalent {1}", std::to_string(numberOfOutputs), std::to_string(m_ledWizNumber)));

   for (int i = 1; i <= numberOfOutputs; i++)
   {
      LedWizEquivalentOutput* output = m_outputs.GetOrCreateOutput(i);
      if (output != nullptr)
      {
         std::string outputName = "LedWiz " + std::to_string(m_ledWizNumber) + " Output " + std::to_string(i);
         output->SetOutputName(outputName);
      }
   }
}

void LedWizEquivalent::ResolveOutputs(Cabinet* cabinet)
{
   if (cabinet == nullptr)
      return;

   int resolvedCount = 0;

   for (LedWizEquivalentOutput* ledWizOutput : m_outputs)
   {
      if (ledWizOutput == nullptr)
         continue;

      const std::string& outputName = ledWizOutput->GetOutputName();
      if (outputName.empty())
         continue;

      // Resolve the same way the C# original does (Cabinet.Outputs[OutputName]).
      // CabinetOutputList::GetByName understands the "{controller}\{output}" path
      // form the auto-configurators emit, so hierarchical names resolve here too.
      CabinetOutputList* cabinetOutputs = cabinet->GetOutputs();
      IOutput* physicalOutput = cabinetOutputs != nullptr ? cabinetOutputs->GetByName(outputName) : nullptr;

      if (physicalOutput != nullptr)
      {
         ledWizOutput->SetOutput(physicalOutput);
         resolvedCount++;
      }
      else
      {
         if (cabinet->GetToys() != nullptr)
         {
            bool foundToy = false;
            for (IToy* toy : *cabinet->GetToys())
            {
               if (toy != nullptr && toy->GetName() == outputName)
               {
                  foundToy = true;
                  break;
               }
            }
            if (!foundToy)
            {
               Log::Warning(StringExtensions::Build("LedWizEquivalent {0}: Could not resolve output '{1}'", std::to_string(m_ledWizNumber), outputName));
            }
         }
         else
         {
            Log::Warning(StringExtensions::Build("LedWizEquivalent {0}: Could not resolve output '{1}'", std::to_string(m_ledWizNumber), outputName));
         }
      }
   }

   Log::Write(StringExtensions::Build(
      "LedWizEquivalent {0}: Resolved {1} of {2} outputs", std::to_string(m_ledWizNumber), std::to_string(resolvedCount), std::to_string(static_cast<int>(m_outputs.size()))));
}

tinyxml2::XMLElement* LedWizEquivalentOutput::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement("LedWizEquivalentOutput");

   tinyxml2::XMLElement* outputNameElement = doc.NewElement("OutputName");
   outputNameElement->SetText(m_outputName.c_str());
   element->InsertEndChild(outputNameElement);

   tinyxml2::XMLElement* numberElement = doc.NewElement("LedWizEquivalentOutputNumber");
   numberElement->SetText(m_ledWizEquivalentOutputNumber);
   element->InsertEndChild(numberElement);

   return element;
}

bool LedWizEquivalentOutput::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const tinyxml2::XMLElement* outputNameElement = element->FirstChildElement("OutputName");
   if (outputNameElement && outputNameElement->GetText())
      m_outputName = outputNameElement->GetText();

   const tinyxml2::XMLElement* numberElement = element->FirstChildElement("LedWizEquivalentOutputNumber");
   if (numberElement && numberElement->GetText())
   {
      try
      {
         m_ledWizEquivalentOutputNumber = std::stoi(numberElement->GetText());
      }
      catch (...)
      {
         return false;
      }
   }

   return true;
}

tinyxml2::XMLElement* LedWizEquivalentOutputList::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* outputsElement = doc.NewElement("Outputs");

   for (const LedWizEquivalentOutput* output : *this)
   {
      if (output)
      {
         tinyxml2::XMLElement* outputElement = output->ToXml(doc);
         if (outputElement)
            outputsElement->InsertEndChild(outputElement);
      }
   }

   return outputsElement;
}

bool LedWizEquivalentOutputList::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   for (const tinyxml2::XMLElement* outputElement = element->FirstChildElement("LedWizEquivalentOutput"); outputElement;
      outputElement = outputElement->NextSiblingElement("LedWizEquivalentOutput"))
   {
      LedWizEquivalentOutput* output = new LedWizEquivalentOutput();
      if (output->FromXml(outputElement))
      {
         push_back(output);
      }
      else
      {
         delete output;
         return false;
      }
   }

   return true;
}

tinyxml2::XMLElement* LedWizEquivalent::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement("LedWizEquivalent");

   tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
   nameElement->SetText(GetName().c_str());
   element->InsertEndChild(nameElement);

   tinyxml2::XMLElement* outputsElement = m_outputs.ToXml(doc);
   if (outputsElement)
      element->InsertEndChild(outputsElement);

   tinyxml2::XMLElement* numberElement = doc.NewElement("LedWizNumber");
   numberElement->SetText(m_ledWizNumber);
   element->InsertEndChild(numberElement);

   return element;
}

bool LedWizEquivalent::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const tinyxml2::XMLElement* nameElement = element->FirstChildElement("Name");
   if (nameElement && nameElement->GetText())
      SetName(nameElement->GetText());

   const tinyxml2::XMLElement* outputsElement = element->FirstChildElement("Outputs");
   if (outputsElement)
   {
      if (!m_outputs.FromXml(outputsElement))
         return false;
   }

   const tinyxml2::XMLElement* numberElement = element->FirstChildElement("LedWizNumber");
   if (numberElement && numberElement->GetText())
   {
      try
      {
         m_ledWizNumber = std::stoi(numberElement->GetText());
      }
      catch (...)
      {
         return false;
      }
   }

   return true;
}

}