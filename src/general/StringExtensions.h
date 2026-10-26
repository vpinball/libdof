#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <charconv>

namespace DOF
{

class StringExtensions
{
public:
   static std::string ToAddressString(const void* ptr);
   static int ToInteger(const std::string& s);
   static bool IsInteger(const std::string& s);
   static bool IsNullOrEmpty(const std::string& s);
   static bool IsNullOrWhiteSpace(const std::string& s);
   static std::string ToUpper(const std::string& s);
   static std::string ToLower(const std::string& s);
   static std::string Trim(const std::string& s);
   static bool StartsWith(const std::string& s, const std::string& prefix);
   static bool EndsWith(const std::string& s, const std::string& suffix);
   static bool EqualsIgnoreCase(const std::string& s1, const std::string& s2);
   static std::string Replace(const std::string& s, const std::string& from, const std::string& to);
   static std::vector<std::string> Split(const std::string& s, const std::vector<char>& delimiters);
   static void WriteToFile(const std::string& data, const std::string& filename);
   static void WriteToFile(const std::string& data, const std::string& filename, bool append);
   static std::string Build(const std::string& format, const std::string& arg0);
   static std::string Build(const std::string& format, const std::string& arg0, const std::string& arg1);
   static std::string Build(const std::string& format, const std::string& arg0, const std::string& arg1, const std::string& arg2);
   static std::string Build(const std::string& format, const std::string& arg0, const std::string& arg1, const std::string& arg2, const std::string& arg3);
   static std::string Build(const std::string& format, const std::vector<std::string>& args);
   static bool TryParseInt(const std::string& str, int& value);
   static bool IsHexString(const std::string& s);
   static int HexToInt(const std::string& s);
   static std::string FormatNumber(int value, int padding);
   static std::string ToBase64(const std::vector<uint8_t>& data);
   static std::vector<uint8_t> FromBase64(const std::string& base64);
};

}