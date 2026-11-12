#pragma once

#include "../IAutoConfigOutputController.h"
#include <string>
#include <cstdint>

namespace DOF
{

class Cabinet;

class PinOneAutoConfigurator : public IAutoConfigOutputController
{
public:
   virtual void AutoConfig(Cabinet* cabinet) override;
   static std::string GetDevice();

private:
   static const std::string LegacyHardwareId;

   static std::string FindLegacyComPort(bool& wmiAvailable);
   static bool TryConnect(const std::string& sp);
};

}
