#pragma once

#include "FileInfo.h"

#include <unordered_map>
#include <vector>

namespace DOF
{

class FilePattern
{
public:
   FilePattern();
   FilePattern(const std::string& pattern);

   const std::string& GetPattern() const noexcept;
   void SetPattern(const std::string& pattern);

   std::string ReplacePlaceholders(const std::unordered_map<std::string, std::string>& replaceValues = { }) const;
   std::vector<FileInfo> GetMatchingFiles(const std::unordered_map<std::string, std::string>& replaceValues = { }) const;
   FileInfo* GetFirstMatchingFile(const std::unordered_map<std::string, std::string>& replaceValues = { }) const;

   bool IsValid() const noexcept;

private:
   std::string m_pattern;
};

}
