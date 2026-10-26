#include "DirectOutputHandler.h"
#include "Log.h"
#include "general/DirectoryInfo.h"
#include "general/FileInfo.h"
#include "general/Path.h"
#include "general/StringExtensions.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#elif defined(__linux__)
#include <unistd.h>
#include <climits>
#endif

namespace DOF
{

std::string DirectOutputHandler::GetInstallFolder()
{
   std::string executablePath;

#ifdef _WIN32
   char buffer[MAX_PATH];
   DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
   if (length > 0 && length < MAX_PATH)
   {
      executablePath = std::string(buffer, length);
   }
#elif defined(__APPLE__)
   char buffer[PATH_MAX];
   uint32_t size = PATH_MAX;
   if (_NSGetExecutablePath(buffer, &size) == 0)
   {
      executablePath = std::string(buffer);
   }
#elif defined(__linux__)
   char buffer[PATH_MAX];
   ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX - 1);
   if (length != -1)
   {
      buffer[length] = '\0';
      executablePath = std::string(buffer);
   }
#endif

   if (executablePath.empty())
      return "";

   std::string assemblyPath = FileInfo(executablePath).Directory().FullName();

   DirectoryInfo assemblyConfigPath(Path::Combine(assemblyPath, "Config"));
   DirectoryInfo assemblyParentConfigPath(Path::GetFullPath(Path::Combine(assemblyPath, "../Config")));
   if (!assemblyConfigPath.Exists() && assemblyParentConfigPath.Exists())
   {
      std::string parent = Path::GetDirectoryName(assemblyPath);
      Log::Once("InstallFolderLoc",
         StringExtensions::Build("Install folder lookup: executable: {0}, install folder: {1} (PARENT of the executable folder -> new shared x86/x64 install)", executablePath, parent));
      return parent;
   }
   else
   {
      Log::Once("InstallFolderLoc",
         StringExtensions::Build("Install folder lookup: executable: {0}, install folder: {1} (EXECUTABLE folder -> original flat install configuration)", executablePath, assemblyPath));
      return assemblyPath;
   }
}

DirectOutputHandler::DirectOutputHandler() { }
DirectOutputHandler::~DirectOutputHandler() { }

}