#include "DOF/DOF.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

#include "DOF/Config.h"
#include "Log.h"
#include "Logger.h"
#include "Pinball.h"
#include "general/FileInfo.h"
#include "general/IOConfigurator.h"
#include "general/StringExtensions.h"
#include "globalconfiguration/GlobalConfig.h"
#include "ledcontrol/loader/LedControlConfig.h"
#include "ledcontrol/loader/LedControlConfigList.h"
#include "ledcontrol/loader/TableConfig.h"
#include "ledcontrol/loader/TableConfigList.h"

namespace DOF
{

DOF::DOF()
   : m_ledControlConfigs(nullptr)
{
   IOConfigurator::Initialize();
   m_pinball = new Pinball();
}

DOF::~DOF()
{
   delete m_ledControlConfigs;
   delete m_pinball;
   IOConfigurator::Shutdown();
}

static std::string FindGlobalConfigPath()
{
   std::string globalConfigPath = std::string(Config::GetInstance()->GetBasePath()) + "directoutputconfig" + PATH_SEPARATOR_CHAR + "GlobalConfig_B2SServer.xml";
   if (std::filesystem::exists(globalConfigPath))
      return globalConfigPath;
   globalConfigPath = std::string(".") + PATH_SEPARATOR_CHAR + "directoutputconfig" + PATH_SEPARATOR_CHAR + "GlobalConfig_B2SServer.xml";
   if (std::filesystem::exists(globalConfigPath))
      return globalConfigPath;
   return "";
}

void DOF::Init(const char* tableFilename, const char* romName)
{
   std::string globalConfigPath = FindGlobalConfigPath();
   if (!globalConfigPath.empty())
      Log::Write(StringExtensions::Build("Global configuration found at: {0}", globalConfigPath));
   else
   {
      globalConfigPath = std::string(Config::GetInstance()->GetBasePath()) + "directoutputconfig" + PATH_SEPARATOR_CHAR + "GlobalConfig_B2SServer.xml";
      Log::Warning(StringExtensions::Build("Unable to find global configuration. Defaulting to: {0}", globalConfigPath));
   }

   LedControlConfigList* ledControlConfigs = (m_ledControlConfigs && m_ledControlTableFilename == (tableFilename ? tableFilename : "")) ? m_ledControlConfigs : nullptr;
   m_pinball->Setup(globalConfigPath, tableFilename, romName, ledControlConfigs);
   delete m_ledControlConfigs;
   m_ledControlConfigs = nullptr;

   m_pinball->Init();
}

void DOF::DataReceive(char type, int number, int value) { m_pinball->ReceiveData(type, number, value); }

void DOF::Finish() { m_pinball->Finish(); }

std::vector<std::string> DOF::GetLedControlRomNames(const char* tableFilename)
{
   std::string ledControlTableFilename = tableFilename ? tableFilename : "";
   if (!m_ledControlConfigs || m_ledControlTableFilename != ledControlTableFilename)
   {
      delete m_ledControlConfigs;
      m_ledControlConfigs = new LedControlConfigList();
      m_ledControlTableFilename = ledControlTableFilename;

      std::string globalConfigPath = FindGlobalConfigPath();
      GlobalConfig* globalConfig = globalConfigPath.empty() ? nullptr : GlobalConfig::GetGlobalConfigFromConfigXmlFile(FileInfo(globalConfigPath).FullName());
      if (!globalConfig)
         globalConfig = new GlobalConfig();

      std::unordered_map<int, FileInfo> ledControlIniFiles = globalConfig->GetIniFilesDictionary(ledControlTableFilename);
      if (!ledControlIniFiles.empty())
         m_ledControlConfigs->LoadLedControlFiles(ledControlIniFiles, false);
      delete globalConfig;
   }

   std::vector<std::string> romNames;
   for (LedControlConfig* ledControlConfig : *m_ledControlConfigs)
   {
      TableConfigList* tableConfigs = ledControlConfig->GetTableConfigurations();
      for (size_t i = 0; i < tableConfigs->Size(); i++)
         romNames.push_back((*tableConfigs)[i]->GetShortRomName());
   }
   return romNames;
}

}