#include "StringExtensions.h"
#include <iomanip>
#include <algorithm>

namespace DOF
{

std::string StringExtensions::ToAddressString(const void* ptr)
{
   std::ostringstream oss;
   oss << "0x" << std::uppercase << std::hex << reinterpret_cast<std::uintptr_t>(ptr);
   return oss.str();
}

int StringExtensions::ToInteger(const std::string& s)
{
   int value = 0;
   TryParseInt(s, value);
   return value;
}

bool StringExtensions::IsInteger(const std::string& s)
{
   if (s.empty())
      return false;

   size_t start = 0;
   if (s[0] == '-')
      start = 1;

   return std::all_of(s.begin() + start, s.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool StringExtensions::IsNullOrEmpty(const std::string& s) { return s.empty(); }

bool StringExtensions::IsNullOrWhiteSpace(const std::string& s)
{
   return s.empty() || std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
}

std::string StringExtensions::ToUpper(const std::string& s)
{
   std::string result = s;
   std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
   return result;
}

std::string StringExtensions::ToLower(const std::string& s)
{
   std::string result = s;
   std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
   return result;
}

std::string StringExtensions::Trim(const std::string& s)
{
   if (s.empty())
      return s;

   size_t start = 0;
   size_t end = s.length();

   while (start < end && std::isspace(static_cast<unsigned char>(s[start])))
      start++;

   while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
      end--;

   return s.substr(start, end - start);
}

bool StringExtensions::StartsWith(const std::string& s, const std::string& prefix)
{
   if (prefix.length() > s.length())
      return false;
   return s.substr(0, prefix.length()) == prefix;
}

bool StringExtensions::EndsWith(const std::string& s, const std::string& suffix)
{
   if (suffix.length() > s.length())
      return false;
   return s.substr(s.length() - suffix.length()) == suffix;
}

bool StringExtensions::EqualsIgnoreCase(const std::string& s1, const std::string& s2)
{
   if (s1.length() != s2.length())
      return false;

   return std::equal(s1.begin(), s1.end(), s2.begin(), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
}

std::string StringExtensions::Replace(const std::string& s, const std::string& from, const std::string& to)
{
   if (from.empty())
      return s;

   std::string result = s;
   size_t pos = 0;
   while ((pos = result.find(from, pos)) != std::string::npos)
   {
      result.replace(pos, from.length(), to);
      pos += to.length();
   }
   return result;
}

std::vector<std::string> StringExtensions::Split(const std::string& s, const std::vector<char>& delimiters)
{
   std::vector<std::string> result;
   if (s.empty())
      return result;

   size_t start = 0;
   size_t pos = 0;

   while (pos < s.length())
   {
      bool found = false;
      for (char delimiter : delimiters)
      {
         if (s[pos] == delimiter)
         {
            found = true;
            break;
         }
      }

      if (found)
      {
         if (pos > start)
         {
            result.push_back(s.substr(start, pos - start));
         }
         start = pos + 1;
      }
      pos++;
   }

   if (start < s.length())
   {
      result.push_back(s.substr(start));
   }

   return result;
}

void StringExtensions::WriteToFile(const std::string& data, const std::string& filename) { WriteToFile(data, filename, false); }

void StringExtensions::WriteToFile(const std::string& data, const std::string& filename, bool append)
{
   std::ofstream ofs;
   ofs.exceptions(std::ios::failbit | std::ios::badbit);
   ofs.open(filename, append ? (std::ios::out | std::ios::app) : std::ios::out);
   ofs << data;
}

std::string FormatArgument(const std::string& arg, const std::string& formatSpec)
{
   if (formatSpec.empty())
      return arg;

   if (formatSpec.length() > 0 && formatSpec[0] == '0')
   {
      int width = formatSpec.length();
      if (arg.length() < width)
         return std::string(width - arg.length(), '0') + arg;
   }

   return arg;
}

std::string ReplaceArgument(const std::string& format, int argIndex, const std::string& arg)
{
   std::string result = format;
   std::string pattern = "{" + std::to_string(argIndex);

   size_t pos = 0;
   while ((pos = result.find(pattern, pos)) != std::string::npos)
   {
      size_t endPos = result.find("}", pos);
      if (endPos == std::string::npos)
         break;

      std::string placeholder = result.substr(pos, endPos - pos + 1);
      std::string formattedArg = arg;

      size_t colonPos = placeholder.find(":");
      if (colonPos != std::string::npos)
      {
         std::string formatSpec = placeholder.substr(colonPos + 1, placeholder.length() - colonPos - 2);
         formattedArg = FormatArgument(arg, formatSpec);
      }

      result.replace(pos, endPos - pos + 1, formattedArg);
      pos += formattedArg.length();
   }
   return result;
}

std::string StringExtensions::Build(const std::string& format, const std::string& arg0) { return ReplaceArgument(format, 0, arg0); }

std::string StringExtensions::Build(const std::string& format, const std::string& arg0, const std::string& arg1)
{
   std::string result = ReplaceArgument(format, 0, arg0);
   result = ReplaceArgument(result, 1, arg1);
   return result;
}

std::string StringExtensions::Build(const std::string& format, const std::string& arg0, const std::string& arg1, const std::string& arg2)
{
   std::string result = ReplaceArgument(format, 0, arg0);
   result = ReplaceArgument(result, 1, arg1);
   result = ReplaceArgument(result, 2, arg2);
   return result;
}

std::string StringExtensions::Build(const std::string& format, const std::string& arg0, const std::string& arg1, const std::string& arg2, const std::string& arg3)
{
   std::string result = ReplaceArgument(format, 0, arg0);
   result = ReplaceArgument(result, 1, arg1);
   result = ReplaceArgument(result, 2, arg2);
   result = ReplaceArgument(result, 3, arg3);
   return result;
}

std::string StringExtensions::Build(const std::string& format, const std::vector<std::string>& args)
{
   std::string result = format;
   for (size_t i = 0; i < args.size(); ++i)
   {
      result = ReplaceArgument(result, static_cast<int>(i), args[i]);
   }
   return result;
}

std::string StringExtensions::GetFileNameWithoutExtension(const std::string& fullPath) { return std::filesystem::path(fullPath).stem().string(); }

bool StringExtensions::TryParseInt(const std::string& str, int& value)
{
   std::string s = Trim(str);
   if (s.empty())
      return false;
   const char* first = s.c_str();
   const char* last = s.c_str() + s.length();
   if (*first == '+')
   {
      ++first;
      if (first == last || *first == '-' || *first == '+')
         return false;
   }
   std::from_chars_result result = std::from_chars(first, last, value);
   return result.ec == std::errc { } && result.ptr == last;
}

bool StringExtensions::IsHexString(const std::string& s)
{
   if (s.empty())
      return false;
   return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); });
}

int StringExtensions::HexToInt(const std::string& s)
{
   int result;
   std::istringstream iss(s);
   iss >> std::hex >> result;
   return result;
}

std::string StringExtensions::FormatNumber(int value, int padding)
{
   std::ostringstream oss;
   oss << std::setfill('0') << std::setw(padding) << value;
   return oss.str();
}

std::string StringExtensions::ToBase64(const std::vector<uint8_t>& data)
{
   static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   std::string result;
   int i = 0;
   unsigned char char_array_3[3];
   unsigned char char_array_4[4];

   for (auto byte : data)
   {
      char_array_3[i++] = byte;
      if (i == 3)
      {
         char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
         char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
         char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
         char_array_4[3] = char_array_3[2] & 0x3f;

         for (i = 0; i < 4; i++)
            result += chars[char_array_4[i]];
         i = 0;
      }
   }

   if (i)
   {
      for (int j = i; j < 3; j++)
         char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (int j = 0; j < i + 1; j++)
         result += chars[char_array_4[j]];

      while (i++ < 3)
         result += '=';
   }

   return result;
}

std::vector<uint8_t> StringExtensions::FromBase64(const std::string& base64)
{
   static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   std::vector<uint8_t> result;
   int in_len = base64.length();
   int i = 0;
   int in = 0;
   unsigned char char_array_4[4], char_array_3[3];

   while (in_len-- && (base64[in] != '=') && (isalnum(base64[in]) || (base64[in] == '+') || (base64[in] == '/')))
   {
      char_array_4[i++] = base64[in];
      in++;
      if (i == 4)
      {
         for (i = 0; i < 4; i++)
            char_array_4[i] = chars.find(char_array_4[i]);

         char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
         char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
         char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

         for (i = 0; i < 3; i++)
            result.push_back(char_array_3[i]);
         i = 0;
      }
   }

   if (i)
   {
      for (int j = i; j < 4; j++)
         char_array_4[j] = 0;

      for (int j = 0; j < 4; j++)
         char_array_4[j] = chars.find(char_array_4[j]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (int j = 0; j < i - 1; j++)
         result.push_back(char_array_3[j]);
   }

   return result;
}

}