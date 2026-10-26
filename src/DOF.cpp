#include "DOF/DOF.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "DOF/Config.h"
#include "Log.h"
#include "Logger.h"
#include "Pinball.h"
#include "general/FileInfo.h"
#include "general/IOConfigurator.h"
#include "general/Path.h"
#include "general/StringExtensions.h"
#include "globalconfiguration/GlobalConfig.h"
#include "ledcontrol/loader/LedControlConfig.h"
#include "ledcontrol/loader/LedControlConfigList.h"
#include "ledcontrol/loader/TableConfig.h"
#include "ledcontrol/loader/TableConfigList.h"

namespace DOF
{

DOF::DOF()
{
   IOConfigurator::Initialize();
   m_pinball = new Pinball();
}

DOF::~DOF()
{
   delete m_pinball;
   IOConfigurator::Shutdown();
}

static std::string GetGlobalConfigPath()
{
   std::string defaultGlobalConfigPath = Path::Combine(Config::GetInstance()->GetBasePath(), "directoutputconfig", "GlobalConfig_B2SServer.xml");
   if (FileInfo(defaultGlobalConfigPath).Exists())
      return defaultGlobalConfigPath;
   std::string globalConfigPath = Path::Combine(".", "directoutputconfig", "GlobalConfig_B2SServer.xml");
   if (FileInfo(globalConfigPath).Exists())
      return globalConfigPath;
   return defaultGlobalConfigPath;
}

void DOF::Init(const char* tableFilename, const char* romName)
{
   std::string globalConfigPath = GetGlobalConfigPath();
   if (FileInfo(globalConfigPath).Exists())
      Log::Write(StringExtensions::Build("Global configuration found at: {0}", globalConfigPath));
   else
      Log::Warning(StringExtensions::Build("Unable to find global configuration. Defaulting to: {0}", globalConfigPath));

   m_pinball->Setup(globalConfigPath, tableFilename, romName);
   m_pinball->Init();
}

void DOF::DataReceive(char type, int number, int value) { m_pinball->ReceiveData(type, number, value); }

void DOF::Finish() { m_pinball->Finish(); }

std::vector<std::string> DOF::GetLedControlRomNames(const char* tableFilename)
{
   std::string globalConfigPath = GetGlobalConfigPath();
   GlobalConfig* globalConfig = FileInfo(globalConfigPath).Exists() ? GlobalConfig::GetGlobalConfigFromConfigXmlFile(FileInfo(globalConfigPath).FullName()) : nullptr;
   if (!globalConfig)
      globalConfig = new GlobalConfig();
   globalConfig->SetGlobalConfigFilename(globalConfigPath);

   std::unordered_map<int, FileInfo> ledControlIniFiles = globalConfig->GetIniFilesDictionary(tableFilename ? tableFilename : "");

   LedControlConfigList* l = new LedControlConfigList();
   if (ledControlIniFiles.size() > 0)
      l->LoadLedControlFiles(ledControlIniFiles, false);

   std::vector<std::string> romNames;
   for (LedControlConfig* ledControlConfig : *l)
   {
      TableConfigList* tableConfigs = ledControlConfig->GetTableConfigurations();
      for (size_t i = 0; i < tableConfigs->Size(); i++)
         romNames.push_back((*tableConfigs)[i]->GetShortRomName());
   }
   delete l;
   delete globalConfig;
   return romNames;
}

}