#include "ScheduledSettings.h"
#include "ScheduledSettingDevice.h"
#include "../../general/MathExtensions.h"
#include "../out/IOutput.h"
#include "../out/Output.h"
#include "../../Log.h"
#include "../../general/StringExtensions.h"
#include <algorithm>
#include <sstream>

namespace DOF
{

ScheduledSettings& ScheduledSettings::GetInstance()
{
   static ScheduledSettings instance;
   return instance;
}

ScheduledSettings::~ScheduledSettings() { Clear(); }

void ScheduledSettings::Clear()
{
   for (ScheduledSetting* setting : *this)
      delete setting;
   clear();
   m_cacheList.clear();
}

ScheduledSetting* ScheduledSettings::GetActiveSchedule(const std::string& configPostfixID, int outputNumber) const
{
   return GetActiveSchedule(configPostfixID, outputNumber, std::chrono::system_clock::now());
}

ScheduledSetting* ScheduledSettings::GetActiveSchedule(const std::string& configPostfixID, int outputNumber, const std::chrono::system_clock::time_point& now) const { return nullptr; }

uint8_t ScheduledSettings::GetNewRecalculatedOutput(const std::string& configPostfixID, int outputNumber, uint8_t originalOutput) const
{
   return GetNewRecalculatedOutput(configPostfixID, outputNumber, originalOutput, std::chrono::system_clock::now());
}

uint8_t ScheduledSettings::GetNewRecalculatedOutput(const std::string& configPostfixID, int outputNumber, uint8_t originalOutput, const std::chrono::system_clock::time_point& now) const
{
   return originalOutput;
}

tinyxml2::XMLElement* ScheduledSettings::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   for (const ScheduledSetting* setting : *this)
   {
      tinyxml2::XMLElement* settingElement = setting->ToXml(doc);
      if (settingElement)
         element->InsertEndChild(settingElement);
   }

   return element;
}

bool ScheduledSettings::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   Clear();

   const tinyxml2::XMLElement* settingElement = element->FirstChildElement("ScheduledSetting");
   while (settingElement)
   {
      ScheduledSetting* setting = new ScheduledSetting();
      if (setting->FromXml(settingElement))
      {
         push_back(setting);
      }
      else
      {
         delete setting;
      }

      settingElement = settingElement->NextSiblingElement("ScheduledSetting");
   }

   return true;
}

ScheduledSettingDevice* ScheduledSettings::GetActiveSchedule(IOutput* currentOutput, bool recalculateOutputValue, int startingDeviceIndex, int currentDeviceIndex)
{
   if (!currentOutput)
      return nullptr;

   std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
   int deviceID = startingDeviceIndex + currentDeviceIndex;

   for (ScheduledSetting* scheduledSetting : *this)
   {
      if (!scheduledSetting || !scheduledSetting->IsEnabled())
         continue;

      if (!scheduledSetting->IsTimeInRange(now))
         continue;

      const ScheduledSettingDeviceList& deviceList = scheduledSetting->GetScheduledSettingDeviceList();
      for (ScheduledSettingDevice* device : deviceList)
      {
         if (!device)
            continue;

         if (device->GetConfigPostfixID() != deviceID)
            continue;

         const std::vector<int>& outputList = device->GetOutputList();
         int outputNumber = currentOutput->GetNumber();

         if (!outputList.empty())
         {
            if (std::find(outputList.begin(), outputList.end(), outputNumber) == outputList.end())
               continue;
         }

         if (recalculateOutputValue)
         {
            int outputPercent = device->GetOutputPercent();
            int originalValue = currentOutput->GetOutput();
            int newValue = (originalValue * outputPercent) / 100;
            newValue = MathExtensions::Limit(newValue, 0, 255);

            if (std::find(m_cacheList.begin(), m_cacheList.end(), device) == m_cacheList.end())
            {
               m_cacheList.push_back(device);

               if (originalValue != 0)
               {
                  std::ostringstream strengthFactor;
                  strengthFactor << (outputPercent / 100.0);
                  Log::Write(StringExtensions::Build("ScheduledSettings.GetActiveSchedule: found active schedule: {0} [{1}-{2}] at channel #{3} on device config {4}, applying strength multiplier: {5}, old value={6}, new value={7}",
                     { scheduledSetting->GetName(), scheduledSetting->GetClockStart(), scheduledSetting->GetClockEnd(), std::to_string(outputNumber), device->GetName(), strengthFactor.str(),
                        std::to_string(originalValue), std::to_string(newValue) }));
               }
            }

            if (outputPercent != 100)
               currentOutput->SetOutput(static_cast<uint8_t>(newValue));
         }

         return device;
      }
   }

   return nullptr;
}

IOutput* ScheduledSettings::GetNewRecalculatedOutput(IOutput* currentOutput, int startingDeviceIndex, int currentDeviceIndex)
{
   if (currentOutput->GetOutput() != 0)
   {
      Output* newOutput = new Output();
      newOutput->SetOutput(currentOutput->GetOutput());
      newOutput->SetNumber(currentOutput->GetNumber());

      ScheduledSettingDevice* activeScheduleDevice = GetActiveSchedule(newOutput, true, startingDeviceIndex, currentDeviceIndex);

      if (activeScheduleDevice)
         return newOutput;
      else
      {
         delete newOutput;
         return currentOutput;
      }
   }
   else
   {
      return currentOutput;
   }
}

}