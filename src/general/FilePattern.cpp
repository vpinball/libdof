#include "FilePattern.h"

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
   std::vector<FileInfo> out;
   if (m_pattern.empty())
      return out;

   try
   {
      auto pattern = ReplacePlaceholders(replaceValues);
      std::filesystem::path p(pattern);
      std::filesystem::path dir = p.has_parent_path() ? p.parent_path() : std::filesystem::current_path();
      if (!std::filesystem::exists(dir))
         return out;
      auto namePattern = p.filename().string();

      for (auto const& entry : std::filesystem::directory_iterator(dir))
      {
         if (entry.is_regular_file())
         {
            auto filename = entry.path().filename().string();
            if (MatchesPattern(filename, namePattern))
               out.emplace_back(entry.path().string());
         }
      }
   }
   catch (...)
   {
   }
   return out;
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

bool FilePattern::MatchesPattern(const std::string& filename, const std::string& pattern) const
{
   size_t fi = 0, pi = 0;
   size_t starIdx = std::string::npos, match = 0;

   while (fi < filename.length())
   {
      if (pi < pattern.length() && (pattern[pi] == '?' || pattern[pi] == filename[fi]))
      {
         fi++;
         pi++;
      }
      else if (pi < pattern.length() && pattern[pi] == '*')
      {
         starIdx = pi;
         match = fi;
         pi++;
      }
      else if (starIdx != std::string::npos)
      {
         pi = starIdx + 1;
         match++;
         fi = match;
      }
      else
      {
         return false;
      }
   }

   while (pi < pattern.length() && pattern[pi] == '*')
      pi++;

   return pi == pattern.length();
}
}