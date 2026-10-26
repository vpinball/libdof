#include "GlobalConfig.h"

#include "../Log.h"
#include "../DirectOutputHandler.h"
#include "../general/FileReader.h"
#include "../general/DirectoryInfo.h"
#include "../general/Path.h"
#include "../general/StringExtensions.h"

#include <tinyxml2/tinyxml2.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#endif


namespace DOF
{

GlobalConfig::GlobalConfig()
   : m_ledWizDefaultMinCommandIntervalMs(10)
   , m_ledControlMinimumEffectDurationMs(60)
   , m_ledControlMinimumRGBEffectDurationMs(120)
   , m_pacLedDefaultMinCommandIntervalMs(10)
   , m_enableLog(true)
   , m_clearLogOnSessionStart(true)
   , m_instrumentation("")
{
   m_tableConfigFilePatterns.clear();
   m_logFilePattern.SetPattern("./DirectOutput.log");
}

void GlobalConfig::SetLedWizDefaultMinCommandIntervalMs(int value) { m_ledWizDefaultMinCommandIntervalMs = std::clamp(value, 0, 1000); }

void GlobalConfig::SetPacLedDefaultMinCommandIntervalMs(int value) { m_pacLedDefaultMinCommandIntervalMs = std::clamp(value, 0, 1000); }

std::unordered_map<int, FileInfo> GlobalConfig::GetIniFilesDictionary(const std::string& tableFilename) const
{
   std::vector<std::string> lookupPaths;

   if (!StringExtensions::IsNullOrWhiteSpace(m_iniFilesPath))
   {
      try
      {
         DirectoryInfo di(m_iniFilesPath);
         if (di.Exists())
            lookupPaths.push_back(di.FullName());
      }
      catch (...)
      {
         Log::Exception(StringExtensions::Build("The specified IniFilesPath {0} could not be used due to a exception.", m_iniFilesPath));
      }
   }

   if (!StringExtensions::IsNullOrWhiteSpace(tableFilename))
   {
      try
      {
         if (FileInfo(tableFilename).Directory().Exists())
            lookupPaths.push_back(FileInfo(tableFilename).Directory().FullName());
      }

      catch (...)
      {
      }
   }

   try
   {
      DirectoryInfo gcd = GetGlobalConfigDirectory();
      if (gcd.Exists())
         lookupPaths.push_back(gcd.FullName());
   }
   catch (...)
   {
   }

   lookupPaths.push_back(std::filesystem::current_path().string());

   std::unordered_map<int, FileInfo> iniFiles;

   bool foundIt = false;
   const std::string ledControlFilenames[] = { "directoutputconfig", "ledcontrol" };

   for (const std::string& ledControlFilename : ledControlFilenames)
   {
      for (const std::string& p : lookupPaths)
      {
         DirectoryInfo di(p);

         std::vector<FileInfo> files;
         for (FileInfo fi : di.EnumerateFiles())
         {
            if (StringExtensions::ToLower(fi.Name()).starts_with(ledControlFilename) && StringExtensions::ToLower(fi.Name()).ends_with(".ini"))
               files.push_back(fi);
         }

         for (FileInfo fi : files)
         {
            if (StringExtensions::ToLower(fi.Name()) == StringExtensions::ToLower(StringExtensions::Build("{0}.ini", ledControlFilename)))
            {
               if (!iniFiles.contains(1))
               {
                  iniFiles.emplace(1, fi);
                  foundIt = true;
               }
               else
               {
                  Log::Warning("Found more than one ini file with for number 1. Likely you have a ini file without a number and and a second one with number 1.");
               }
            }
            else
            {
               std::string f = fi.Name().substr(ledControlFilename.length(), fi.Name().length() - ledControlFilename.length() - 4);
               if (StringExtensions::IsInteger(f))
               {
                  int ledWizNr = -1;
                  if (StringExtensions::TryParseInt(f, ledWizNr))
                  {
                     if (!iniFiles.contains(ledWizNr))
                     {
                        iniFiles.emplace(ledWizNr, fi);
                        foundIt = true;
                     }
                     else
                     {
                        Log::Warning(StringExtensions::Build("Found more than one ini file with number {0}.", std::to_string(ledWizNr)));
                     }
                  }
               }
            }
         }
         if (foundIt)
            break;
      }
      if (foundIt)
         break;
   }

   return iniFiles;
}

FileInfo* GlobalConfig::GetTableMappingFile(const std::string& tableFilename) const
{
   std::unordered_map<int, FileInfo> iniFileDict = GetIniFilesDictionary(tableFilename);

   if (iniFileDict.size() > 0)
   {
      auto [firstKey, firstValue] = *iniFileDict.begin();
      DirectoryInfo di = firstValue.Directory();
      std::vector<FileInfo> files = di.GetFiles("tablemappings.*");
      if (!files.empty())
         return new FileInfo(files.front().FullName());
      return nullptr;
   }
   else
   {
      return nullptr;
   }
}

FileInfo* GlobalConfig::GetShapeDefinitionFile(const std::string& tableFilename, const std::string& romName) const
{
   if (!StringExtensions::IsNullOrWhiteSpace(m_shapeDefinitionFilePattern.GetPattern()) && m_shapeDefinitionFilePattern.IsValid())
      return m_shapeDefinitionFilePattern.GetFirstMatchingFile(GetReplaceValuesDictionary(tableFilename, romName));

   std::unordered_map<int, FileInfo> iniFilesDict = GetIniFilesDictionary(tableFilename);
   if (iniFilesDict.size() > 0)
   {
      std::string iniFileDir = iniFilesDict.begin()->second.Directory().FullName();
      FileInfo* fi = new FileInfo(Path::Combine(iniFileDir, "DirectOutputShapes.xml"));
      if (fi->Exists())
         return fi;
      delete fi;
   }
   std::string filePat = Path::Combine("{DllDir}", "DirectOutputShapes.xml");
   FileInfo* fii = FilePattern(filePat).GetFirstMatchingFile(GetReplaceValuesDictionary(tableFilename, romName));
   if (fii != nullptr && fii->Exists())
      return fii;
   delete fii;

   return nullptr;
}

FileInfo* GlobalConfig::GetCabinetConfigFile() const
{
   if (!StringExtensions::IsNullOrWhiteSpace(m_cabinetConfigFilePattern.GetPattern()) && m_cabinetConfigFilePattern.IsValid())
      return m_cabinetConfigFilePattern.GetFirstMatchingFile(GetReplaceValuesDictionary());
   return nullptr;
}

DirectoryInfo GlobalConfig::GetCabinetConfigDirectory() const
{
   FileInfo* cc = GetCabinetConfigFile();
   if (cc != nullptr)
   {
      DirectoryInfo result = cc->Directory();
      delete cc;
      return result;
   }
   return DirectoryInfo("");
}

FileInfo* GlobalConfig::GetTableConfigFile(const std::string& fullTableFilename) const
{
   return m_tableConfigFilePatterns.GetFirstMatchingFile(GetReplaceValuesDictionary(fullTableFilename));
}

std::string GlobalConfig::GetLogFilename(const std::string& tableFilename, const std::string& romName) const
{
   std::unordered_map<std::string, std::string> r = GetReplaceValuesDictionary(tableFilename, romName);
   auto now = std::chrono::system_clock::now();
   std::time_t t = std::chrono::system_clock::to_time_t(now);
   std::tm tm;
#if defined(_WIN32)
   localtime_s(&tm, &t);
#else
   localtime_r(&t, &tm);
#endif

   char buf[32];
   std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
   r.emplace("DateTime", std::string(buf));
   std::strftime(buf, sizeof(buf), "%Y%m%d", &tm);
   r.emplace("Date", std::string(buf));
   std::strftime(buf, sizeof(buf), "%H%M%S", &tm);
   r.emplace("Time", std::string(buf));

   return m_logFilePattern.ReplacePlaceholders(r);
}

std::unordered_map<std::string, std::string> GlobalConfig::GetReplaceValuesDictionary(const std::string& tableFilename, const std::string& romName) const
{
   std::unordered_map<std::string, std::string> d;
   FileInfo* globalConfigFile = GetGlobalConfigFile();
   if (globalConfigFile != nullptr)
   {
      DirectoryInfo gcd = GetGlobalConfigDirectory();
      if (gcd.Exists())
      {
         d.emplace("GlobalConfigDirectory", gcd.FullName());
         d.emplace("GlobalConfigDir", gcd.FullName());
      }
      delete globalConfigFile;
   }

   std::string executablePath;
   try
   {
#ifdef _WIN32
      char path[MAX_PATH];
      DWORD result = GetModuleFileNameA(NULL, path, MAX_PATH);
      if (result > 0)
      {
         executablePath = std::filesystem::path(path).parent_path().string();
      }
      else
      {
         executablePath = std::filesystem::current_path().string();
      }
#elif defined(__APPLE__)
      char path[PATH_MAX];
      uint32_t size = sizeof(path);
      if (_NSGetExecutablePath(path, &size) == 0)
      {
         executablePath = std::filesystem::path(path).parent_path().string();
      }
      else
      {
         executablePath = std::filesystem::current_path().string();
      }
#else
      executablePath = std::filesystem::canonical("/proc/self/exe").parent_path().string();
#endif
   }
   catch (...)
   {
      try
      {
         executablePath = std::filesystem::current_path().string();
      }
      catch (...)
      {
         executablePath = ".";
      }
   }

   FileInfo fi(executablePath);
   DirectoryInfo dir = fi.Directory();
   d.emplace("DllDirectory", dir.FullName());
   d.emplace("DllDir", dir.FullName());
   d.emplace("AssemblyDirectory", dir.FullName());
   d.emplace("AssemblyDir", dir.FullName());

   std::string installFolder = DirectOutputHandler::GetInstallFolder();
   if (!installFolder.empty())
   {
      d.emplace("InstallDir", installFolder);
      d.emplace("BinDir", dir.FullName());
   }

   if (!StringExtensions::IsNullOrWhiteSpace(tableFilename))
   {
      FileInfo fi(tableFilename);
      DirectoryInfo tableDir = fi.Directory();
      if (tableDir.Exists())
      {
         d.emplace("TableDirectory", tableDir.FullName());
         d.emplace("TableDir", tableDir.FullName());
         d.emplace("TableDirectoryName", tableDir.Name());
         d.emplace("TableDirName", tableDir.Name());
      }
      d.emplace("TableName", Path::GetFileNameWithoutExtension(fi.FullName()));
   }

   if (!StringExtensions::IsNullOrWhiteSpace(romName))
      d.emplace("RomName", romName);

   return d;
}

std::string GlobalConfig::GlobalConfigDirectoryName() const
{
   DirectoryInfo di = GetGlobalConfigDirectory();
   if (di.Exists())
      return di.FullName();
   return "";
}

DirectoryInfo GlobalConfig::GetGlobalConfigDirectory() const
{
   FileInfo* fi = GetGlobalConfigFile();
   if (fi == nullptr)
      return DirectoryInfo("");
   DirectoryInfo result = fi->Directory();
   delete fi;
   return result;
}

FileInfo* GlobalConfig::GetGlobalConfigFile() const
{
   if (StringExtensions::IsNullOrWhiteSpace(m_globalConfigFileName))
      return nullptr;
   return new FileInfo(m_globalConfigFileName);
}

GlobalConfig* GlobalConfig::GetGlobalConfigFromConfigXmlFile(const std::string& globalConfigFileName)
{
   try
   {
      if (std::filesystem::exists(globalConfigFileName))
      {
         std::string xml = FileReader::ReadFileToString(globalConfigFileName);

         GlobalConfig* globalConfig = FromXml(xml);
         if (globalConfig != nullptr)
            globalConfig->SetGlobalConfigFilename(globalConfigFileName);
         return globalConfig;
      }
      else
      {
         Log::Error(StringExtensions::Build("Global config file \"{0}\" does not exist; no global config loaded", globalConfigFileName));
         return nullptr;
      }
   }
   catch (...)
   {
      return nullptr;
   }
}

GlobalConfig* GlobalConfig::GetGlobalConfigFromGlobalConfigXml(const std::string& configXml) { return FromXml(configXml); }

void GlobalConfig::SaveGlobalConfig(const std::string& globalConfigFilename)
{
   std::string gcFileName = StringExtensions::IsNullOrWhiteSpace(globalConfigFilename) ? m_globalConfigFileName : globalConfigFilename;
   if (StringExtensions::IsNullOrWhiteSpace(gcFileName))
      throw std::runtime_error("No filename for GlobalConfig file has been supplied. Looking up the filename from the property GlobalConfigFilename failed as well");

   if (FileInfo(gcFileName).Exists())
   {
      auto now = std::chrono::system_clock::now();
      auto t = std::chrono::system_clock::to_time_t(now);
      std::ostringstream ts;
      ts << std::put_time(std::localtime(&t), "%Y-%m-%d %H-%M-%S");
      std::filesystem::copy_file(gcFileName,
         Path::Combine(Path::GetDirectoryName(gcFileName),
            StringExtensions::Build("{1} old (replaced {0}){2}", ts.str(), Path::GetFileNameWithoutExtension(gcFileName), Path::GetExtension(gcFileName))));
   }

   DirectoryInfo gcDirectory = FileInfo(gcFileName).Directory();
   gcDirectory.CreateDirectoryPath();

   StringExtensions::WriteToFile(GetGlobalConfigXml(), gcFileName, false);
}

std::string GlobalConfig::ToXml() const
{
   tinyxml2::XMLDocument doc;
   doc.InsertEndChild(doc.NewDeclaration());

   auto now = std::chrono::system_clock::now();
   auto time_t = std::chrono::system_clock::to_time_t(now);
   std::stringstream ss;
   ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H-%M-%S");

   doc.InsertEndChild(doc.NewComment("Global configuration for the DirectOutput framework."));
   doc.InsertEndChild(doc.NewComment(StringExtensions::Build("Saved by DirectOutput Version libdof-cpp: {0}", ss.str()).c_str()));

   tinyxml2::XMLElement* element = doc.NewElement("GlobalConfig");
   doc.InsertEndChild(element);

   element = doc.NewElement("LedWizDefaultMinCommandIntervalMs");
   element->SetText(m_ledWizDefaultMinCommandIntervalMs);
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("LedControlMinimumEffectDurationMs");
   element->SetText(m_ledControlMinimumEffectDurationMs);
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("LedControlMinimumRGBEffectDurationMs");
   element->SetText(m_ledControlMinimumRGBEffectDurationMs);
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("PacLedDefaultMinCommandIntervalMs");
   element->SetText(m_pacLedDefaultMinCommandIntervalMs);
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("IniFilesPath");
   if (!m_iniFilesPath.empty())
      element->SetText(m_iniFilesPath.c_str());
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("ShapeDefinitionFilePattern");
   if (!m_shapeDefinitionFilePattern.GetPattern().empty())
      element->SetText(m_shapeDefinitionFilePattern.GetPattern().c_str());
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("CabinetConfigFilePattern");
   if (!m_cabinetConfigFilePattern.GetPattern().empty())
      element->SetText(m_cabinetConfigFilePattern.GetPattern().c_str());
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("TableConfigFilePatterns");
   for (auto& fp : m_tableConfigFilePatterns)
   {
      tinyxml2::XMLElement* filePattern = doc.NewElement("FilePattern");
      filePattern->SetText(fp.GetPattern().c_str());
      element->InsertEndChild(filePattern);
   }
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("EnableLogging");
   element->SetText(m_enableLog);
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("ClearLogOnSessionStart");
   element->SetText(m_clearLogOnSessionStart);
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("LogFilePattern");
   element->SetText(m_logFilePattern.GetPattern().c_str());
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   element = doc.NewElement("Instrumentation");
   element->SetText(m_instrumentation.c_str());
   doc.FirstChildElement("GlobalConfig")->InsertEndChild(element);

   tinyxml2::XMLPrinter printer;
   doc.Print(&printer);
   return std::string(printer.CStr());
}

GlobalConfig* GlobalConfig::FromXml(const std::string& configXml)
{
   tinyxml2::XMLDocument doc;
   if (doc.Parse(configXml.c_str()) != tinyxml2::XML_SUCCESS)
   {
      Log::Warning(StringExtensions::Build("GlobalConfig XML parse error: {0}", std::string(doc.ErrorStr() ? doc.ErrorStr() : "unknown error")));
      return nullptr;
   }

   tinyxml2::XMLElement* root = doc.FirstChildElement("GlobalConfig");
   if (!root)
   {
      Log::Warning("GlobalConfig root element not found");
      return nullptr;
   }

   GlobalConfig* globalConfig = new GlobalConfig();

   tinyxml2::XMLElement* element = root->FirstChildElement("LedWizDefaultMinCommandIntervalMs");
   if (element && element->GetText())
   {
      int value;
      if (element->QueryIntText(&value) == tinyxml2::XML_SUCCESS)
         globalConfig->SetLedWizDefaultMinCommandIntervalMs(value);
   }

   element = root->FirstChildElement("LedControlMinimumEffectDurationMs");
   if (element && element->GetText())
   {
      int value;
      if (element->QueryIntText(&value) == tinyxml2::XML_SUCCESS)
         globalConfig->SetLedControlMinimumEffectDurationMs(value);
   }

   element = root->FirstChildElement("LedControlMinimumRGBEffectDurationMs");
   if (element && element->GetText())
   {
      int value;
      if (element->QueryIntText(&value) == tinyxml2::XML_SUCCESS)
         globalConfig->SetLedControlMinimumRGBEffectDurationMs(value);
   }

   element = root->FirstChildElement("PacLedDefaultMinCommandIntervalMs");
   if (element && element->GetText())
   {
      int value;
      if (element->QueryIntText(&value) == tinyxml2::XML_SUCCESS)
         globalConfig->SetPacLedDefaultMinCommandIntervalMs(value);
   }

   element = root->FirstChildElement("IniFilesPath");
   if (element && element->GetText())
      globalConfig->SetIniFilesPath(element->GetText());

   element = root->FirstChildElement("ShapeDefinitionFilePattern");
   if (element && element->GetText())
      globalConfig->SetShapeDefinitionFilePattern(element->GetText());

   element = root->FirstChildElement("CabinetConfigFilePattern");
   if (element && element->GetText())
      globalConfig->SetCabinetConfigFilePattern(element->GetText());

   element = root->FirstChildElement("TableConfigFilePatterns");
   if (element)
   {
      tinyxml2::XMLElement* patternElement = element->FirstChildElement("FilePattern");
      while (patternElement)
      {
         if (patternElement->GetText())
            globalConfig->GetTableConfigFilePatterns().push_back(FilePattern(patternElement->GetText()));

         patternElement = patternElement->NextSiblingElement("FilePattern");
      }
   }

   element = root->FirstChildElement("EnableLogging");
   if (element && element->GetText())
   {
      bool value;
      if (element->QueryBoolText(&value) == tinyxml2::XML_SUCCESS)
         globalConfig->SetEnableLogging(value);
   }

   element = root->FirstChildElement("ClearLogOnSessionStart");
   if (element && element->GetText())
   {
      bool value;
      if (element->QueryBoolText(&value) == tinyxml2::XML_SUCCESS)
         globalConfig->SetClearLogOnSessionStart(value);
   }

   element = root->FirstChildElement("LogFilePattern");
   if (element && element->GetText())
      globalConfig->SetLogFilePattern(element->GetText());

   element = root->FirstChildElement("Instrumentation");
   if (element && element->GetText())
      globalConfig->SetInstrumentation(element->GetText());

   return globalConfig;
}

}
