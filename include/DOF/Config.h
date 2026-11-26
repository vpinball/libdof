#pragma once

#if defined(LIBDOF_STATIC)
#define LIBDOFAPI
#elif defined(_MSC_VER)
#define LIBDOFAPI __declspec(dllexport)
#else
#define LIBDOFAPI __attribute__((visibility("default")))
#endif

#ifdef _MSC_VER
#define LIBDOFCALLBACK __stdcall
#else
#define LIBDOFCALLBACK
#endif

#include <cstdarg>
#include <cstdint>
#include <string>

typedef enum
{
   DOF_LogLevel_INFO = 0,
   DOF_LogLevel_WARN = 1,
   DOF_LogLevel_ERROR = 2,
   DOF_LogLevel_DEBUG = 3
} DOF_LogLevel;

typedef void(LIBDOFCALLBACK* DOF_LogCallback)(DOF_LogLevel logLevel, const char* format, va_list args);

namespace DOF
{

class LIBDOFAPI Config
{
public:
   static Config* GetInstance();

   void SetBasePath(const char* basePath) { m_basePath = basePath; }
   const char* GetBasePath() const { return m_basePath.c_str(); }
   DOF_LogLevel GetLogLevel() const { return m_logLevel; }
   void SetLogLevel(DOF_LogLevel logLevel) { m_logLevel = logLevel; }
   DOF_LogCallback GetLogCallback() const { return m_logCallback; }
   void SetLogCallback(DOF_LogCallback callback) { m_logCallback = callback; }

private:
   Config();
   ~Config() { }

   static Config* m_instance;

   std::string m_basePath;
   DOF_LogLevel m_logLevel;
   DOF_LogCallback m_logCallback;
};

} // namespace DOF
