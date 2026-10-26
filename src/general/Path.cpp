#include "Path.h"

#include <filesystem>

namespace DOF
{

std::string Path::Combine(const std::string& path1, const std::string& path2) { return (std::filesystem::path(path1) / path2).string(); }

std::string Path::Combine(const std::string& path1, const std::string& path2, const std::string& path3) { return (std::filesystem::path(path1) / path2 / path3).string(); }

std::string Path::GetDirectoryName(const std::string& path) { return std::filesystem::path(path).parent_path().string(); }

std::string Path::GetFileName(const std::string& path) { return std::filesystem::path(path).filename().string(); }

std::string Path::GetFileNameWithoutExtension(const std::string& path) { return std::filesystem::path(path).stem().string(); }

std::string Path::GetExtension(const std::string& path) { return std::filesystem::path(path).extension().string(); }

std::string Path::GetFullPath(const std::string& path) { return std::filesystem::absolute(path).lexically_normal().string(); }

}
