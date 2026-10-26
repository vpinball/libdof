#pragma once

#include <string>

namespace DOF
{

class Path
{
public:
   static std::string Combine(const std::string& path1, const std::string& path2);
   static std::string Combine(const std::string& path1, const std::string& path2, const std::string& path3);
   static std::string GetDirectoryName(const std::string& path);
   static std::string GetFileName(const std::string& path);
   static std::string GetFileNameWithoutExtension(const std::string& path);
   static std::string GetExtension(const std::string& path);
   static std::string GetFullPath(const std::string& path);
};

}
