#include "FilePattern.h"

#include "DirectoryInfo.h"
#include "Path.h"
#include "StringExtensions.h"

#include <filesystem>

namespace DOF
{

FilePattern::FilePattern() { }

FilePattern::FilePattern(const std::string& pattern)
   : m_pattern(pattern)
{
}

const std::string& FilePattern::GetPattern() const noexcept { return m_pattern; }

void FilePattern::SetPattern(const std::string& pattern)
{
   if (m_pattern != pattern)
      m_pattern = pattern;
}

std::string FilePattern::ReplacePlaceholders(const std::unordered_map<std::string, std::string>& replaceValues) const
{
   std::string result = m_pattern;
   for (auto const& kv : replaceValues)
   {
      std::string ph = "{" + kv.first + "}";
      size_t pos = 0;
      while ((pos = result.find(ph, pos)) != std::string::npos)
      {
         result.replace(pos, ph.size(), kv.second);
         pos += kv.second.size();
      }
   }
   return result;
}

std::vector<FileInfo> FilePattern::GetMatchingFiles(const std::unordered_map<std::string, std::string>& replaceValues) const
{
   if (StringExtensions::IsNullOrWhiteSpace(m_pattern))
      return std::vector<FileInfo>();

   std::string p = ReplacePlaceholders(replaceValues);

   try
   {
      DirectoryInfo filesDirectory(StringExtensions::IsNullOrWhiteSpace(Path::GetDirectoryName(p)) ? "." : Path::GetDirectoryName(p));

      if (filesDirectory.Exists())
         return filesDirectory.GetFiles(Path::GetFileName(p));
      else
         return std::vector<FileInfo>();
   }
   catch (...)
   {
      return std::vector<FileInfo>();
   }
}

FileInfo* FilePattern::GetFirstMatchingFile(const std::unordered_map<std::string, std::string>& replaceValues) const
{
   auto files = GetMatchingFiles(replaceValues);
   if (!files.empty())
      return new FileInfo(files.front().FullName());
   return nullptr;
}

bool FilePattern::IsValid() const noexcept
{
   if (m_pattern.empty())
      return true;
   try
   {
      auto test = m_pattern;
      for (auto& c : test)
         if (c == '*' || c == '?')
            c = 'X';
      std::filesystem::path dummy(test);
   }
   catch (...)
   {
      return false;
   }
   bool bracketOpen = false, justOpened = false;
   for (auto c : m_pattern)
   {
      if (c == '{')
      {
         if (bracketOpen)
            return false;
         bracketOpen = true;
         justOpened = true;
      }
      else if (c == '}')
      {
         if (!bracketOpen || justOpened)
            return false;
         bracketOpen = false;
         justOpened = false;
      }
      else if (c == '\\' || c == '*' || c == '?')
      {
         if (bracketOpen)
            return false;
         justOpened = false;
      }
      else
      {
         justOpened = false;
      }
   }
   return !bracketOpen;
}
}
