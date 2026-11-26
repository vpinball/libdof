#pragma once

#include "ScheduledSetting.h"
#include "../../general/generic/IXmlSerializable.h"
#include <vector>
#include <memory>

namespace DOF
{

class IOutput;
class ScheduledSettingDevice;

class ScheduledSettings : public std::vector<ScheduledSetting*>, public IXmlSerializable
{
public:
   static ScheduledSettings& GetInstance();

   virtual ~ScheduledSettings();

   ScheduledSetting* GetActiveSchedule(const std::string& configPostfixID, int outputNumber) const;
   ScheduledSetting* GetActiveSchedule(const std::string& configPostfixID, int outputNumber, const std::chrono::system_clock::time_point& now) const;
   ScheduledSettingDevice* GetActiveSchedule(IOutput* currentOutput, bool recalculateOutputValue, int startingDeviceIndex, int currentDeviceIndex);

   uint8_t GetNewRecalculatedOutput(const std::string& configPostfixID, int outputNumber, uint8_t originalOutput) const;
   uint8_t GetNewRecalculatedOutput(const std::string& configPostfixID, int outputNumber, uint8_t originalOutput, const std::chrono::system_clock::time_point& now) const;

   IOutput* GetNewRecalculatedOutput(IOutput* currentOutput, int startingDeviceIndex, int currentDeviceIndex);


   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "ScheduledSettings"; }
   void Clear();

private:
   ScheduledSettings() = default;
   ScheduledSettings(const ScheduledSettings&) = delete;
   ScheduledSettings& operator=(const ScheduledSettings&) = delete;
};

}