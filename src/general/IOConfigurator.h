#pragma once

#include "DOF/DOF.h"

namespace DOF
{

class IOConfigurator
{
public:
   static void Initialize();
   static void Shutdown();

private:
   IOConfigurator() = delete;
};

}
