#include "Log.h"
#include "Logger.h"
#include "general/StringExtensions.h"
#include "../include/DOF/DOF.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace DOF
{

std::ofstream Log::m_logger;
bool Log::m_isInitialized = false;
bool Log::m_isOk = false;
bool Log::m_isEnabled = true;
std::recursive_mutex Log::m_locker;

std::string Log::m_filename = "./DirectOutput.log";
std::string Log::m_instrumentations = "";
std::unordered_set<std::string> Log::m_activeInstrumentations;
std::vector<std::string> Log::m_preLogFileLog;
std::unordered_set<std::string> Log::m_onceKeys;

void Log::SetInstrumentations(const std::string& instrumentations)
{
   m_instrumentations = instrumentations;
   if (!instrumentations.empty())
   {
      m_activeInstrumentations.clear();
      std::stringstream ss(instrumentations);
      std::string item;
      while (std::getline(ss, item, ','))
      {
         item.erase(0, item.find_first_not_of(" \t"));
         item.erase(item.find_last_not_of(" \t") + 1);
         if (!item.empty())
            m_activeInstrumentations.insert(item);
      }
   }
}

void Log::Init(bool enableLogging)
{
   std::lock_guard<std::recursive_mutex> lock(m_locker);
   m_isEnabled = enableLogging;
   if (!m_isInitialized && m_isEnabled)
   {
      try
      {
         std::filesystem::path logPath(m_filename);
         if (logPath.has_parent_path())
         {
            std::filesystem::create_directories(logPath.parent_path());
         }

         m_logger.open(m_filename, std::ios::app);
         if (!m_logger.is_open())
         {
            m_isOk = false;
            m_isInitialized = true;
            return;
         }

         std::string buildConfiguration =
#ifdef _DEBUG
            "Debug";
#else
            "Release";
#endif

         std::string instrumentationsEnabledNote = (!m_activeInstrumentations.empty()) ? StringExtensions::Build("; Instrumentations enabled: {0}", m_instrumentations) : "";

         m_logger << "---------------------------------------------------------------------------------" << std::endl;
         auto now = std::chrono::system_clock::now();
         auto time_t = std::chrono::system_clock::to_time_t(now);
         std::stringstream ss;
         struct tm localTime1;
#ifdef _WIN32
         localtime_s(&localTime1, &time_t);
         ss << std::put_time(&localTime1, "%Y.%m.%d %H:%M");
#else
         ss << std::put_time(localtime_r(&time_t, &localTime1), "%Y.%m.%d %H:%M");
#endif

         m_logger << StringExtensions::Build("libdof (DirectOutput Framework) - Version {0}, built {1}", LIBDOF_VERSION, ss.str()) << std::endl;
         m_logger << "DOF created by SwissLizard / MIT License" << std::endl;
         m_logger << "Original C# version: https://github.com/DirectOutput/DirectOutput" << std::endl;
         m_logger << "C++ port: https://github.com/jsm174/libdof" << std::endl;

         auto nowMs = std::chrono::system_clock::now();
         auto timeT = std::chrono::system_clock::to_time_t(nowMs);
         auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(nowMs.time_since_epoch()) % 1000;
         std::stringstream timestamp;
         struct tm localTime2;
#ifdef _WIN32
         localtime_s(&localTime2, &timeT);
         timestamp << std::put_time(&localTime2, "%Y.%m.%d %H:%M:%S");
#else
         timestamp << std::put_time(localtime_r(&timeT, &localTime2), "%Y.%m.%d %H:%M:%S");
#endif
         timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();

         m_logger << timestamp.str() << "\t" << StringExtensions::Build("DirectOutput logger initialized{0}", instrumentationsEnabledNote) << std::endl;

         m_isOk = true;

         if (!m_preLogFileLog.empty())
         {
            for (const auto& s : m_preLogFileLog)
               m_logger << s << std::endl;

            m_preLogFileLog.clear();
         }

         m_logger.flush();
      }
      catch (...)
      {
         m_isOk = false;
      }

      m_isInitialized = true;
   }
   else if (!m_isInitialized)
   {
      m_isInitialized = true;
      m_isOk = false;
   }
}

void Log::AfterInit() { m_preLogFileLog.clear(); }

void Log::Finish()
{
   std::lock_guard<std::recursive_mutex> lock(m_locker);
   if (m_logger.is_open())
   {
      Write("Logging stopped");
      m_logger.flush();
      m_logger.close();
   }
   m_preLogFileLog.clear();
   m_isOk = false;
   m_isInitialized = false;
   m_isEnabled = true;
}

void Log::WriteRaw(const std::string& message)
{
   std::lock_guard<std::recursive_mutex> lock(m_locker);
   if (!m_isEnabled)
      return;

   if (m_isOk && m_logger.is_open())
   {
      try
      {
         m_logger << message << std::endl;
         m_logger.flush();
      }
      catch (...)
      {
      }
   }
   else if (!m_isInitialized)
   {
      m_preLogFileLog.push_back(message);
   }
}

void Log::Write(const std::string& message)
{
   ::DOF::Log(DOF_LogLevel_INFO, "%s", message.c_str());
   WriteToFile(message);
}

void Log::WriteToFile(const std::string& message)
{
   if (StringExtensions::IsNullOrWhiteSpace(message))
   {
      auto nowMs = std::chrono::system_clock::now();
      auto timeT = std::chrono::system_clock::to_time_t(nowMs);
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(nowMs.time_since_epoch()) % 1000;
      std::stringstream timestamp;
      struct tm localTime3;
#ifdef _WIN32
      localtime_s(&localTime3, &timeT);
      timestamp << std::put_time(&localTime3, "%Y.%m.%d %H:%M:%S");
#else
      timestamp << std::put_time(localtime_r(&timeT, &localTime3), "%Y.%m.%d %H:%M:%S");
#endif
      timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();
      WriteRaw(timestamp.str() + "\t");
   }
   else
   {
      std::stringstream ss(message);
      std::string line;
      while (std::getline(ss, line))
      {
         if (!line.empty() && line.back() == '\r')
            line.pop_back();

         auto nowMs = std::chrono::system_clock::now();
         auto timeT = std::chrono::system_clock::to_time_t(nowMs);
         auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(nowMs.time_since_epoch()) % 1000;
         std::stringstream timestamp;
         struct tm localTime4;
#ifdef _WIN32
         localtime_s(&localTime4, &timeT);
         timestamp << std::put_time(&localTime4, "%Y.%m.%d %H:%M:%S");
#else
         timestamp << std::put_time(localtime_r(&timeT, &localTime4), "%Y.%m.%d %H:%M:%S");
#endif
         timestamp << "." << std::setfill('0') << std::setw(3) << ms.count();
         WriteRaw(timestamp.str() + "\t" + line);
      }
   }
}

void Log::Error(const std::string& message)
{
   ::DOF::Log(DOF_LogLevel_ERROR, "%s", message.c_str());
   WriteToFile(StringExtensions::Build("Error: {0}", message));
}

void Log::Warning(const std::string& message)
{
   ::DOF::Log(DOF_LogLevel_WARN, "%s", message.c_str());
   WriteToFile(StringExtensions::Build("Warning: {0}", message));
}

void Log::Exception(const std::string& message)
{
   ::DOF::Log(DOF_LogLevel_ERROR, "%s", message.c_str());
   WriteToFile(StringExtensions::Build("EXCEPTION: {0}", message));
}

void Log::Debug(const std::string& message)
{
   ::DOF::Log(DOF_LogLevel_DEBUG, "%s", message.c_str());
   WriteToFile(StringExtensions::Build("Debug: {0}", message));
}

void Log::Once(const std::string& key, const std::string& message)
{
   if (m_onceKeys.find(key) == m_onceKeys.end())
   {
      m_onceKeys.insert(key);
      Write(message);
   }
}

void Log::Instrumentation(const std::string& key, const std::string& message)
{
   bool writeMessage = m_activeInstrumentations.find("*") != m_activeInstrumentations.end();
   if (!writeMessage)
   {
      std::stringstream ss(key);
      std::string keyItem;
      bool allKeysFound = true;

      while (std::getline(ss, keyItem, ',') && allKeysFound)
      {
         keyItem.erase(0, keyItem.find_first_not_of(" \t"));
         keyItem.erase(keyItem.find_last_not_of(" \t") + 1);
         if (m_activeInstrumentations.find(keyItem) == m_activeInstrumentations.end())
            allKeysFound = false;
      }
      writeMessage = allKeysFound;
   }

   if (writeMessage)
   {
      ::DOF::Log(DOF_LogLevel_DEBUG, "%s", message.c_str());
      WriteToFile(StringExtensions::Build("Debug [{0}]: {1}", key, message));
   }
}

}
