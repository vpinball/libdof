#pragma once

#include "SequentialOutputSetting.h"
#include "../../general/generic/IXmlSerializable.h"
#include <vector>

namespace DOF
{

class SequentialOutputSettings : public std::vector<SequentialOutputSetting*>, public IXmlSerializable
{
public:
   static SequentialOutputSettings& GetInstance();

   virtual ~SequentialOutputSettings();

   SequentialOutputDevice* GetActiveSequentialDevice(const std::string& configPostfixID, int outputNumber) const;
   int GetNextOutput(const std::string& configPostfixID, int outputNumber, int currentOutputNumber) const;


   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "SequentialOutputSettings"; }
   void Clear();

private:
   SequentialOutputSettings() = default;
   SequentialOutputSettings(const SequentialOutputSettings&) = delete;
   SequentialOutputSettings& operator=(const SequentialOutputSettings&) = delete;
};

}