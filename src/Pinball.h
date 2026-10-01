#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "DOF/DOF.h"

namespace DOF
{

class Cabinet;
class Table;
class AlarmHandler;
class GlobalConfig;
class InputQueue;
class TableElementData;

class Pinball
{
public:
   Pinball();
   ~Pinball();

   void SetTable(Table* table) { m_table = table; }
   Table* GetTable() { return m_table; }
   void SetCabinet(Cabinet* cabinet) { m_cabinet = cabinet; }
   Cabinet* GetCabinet() { return m_cabinet; }
   AlarmHandler* GetAlarms() { return m_alarms; }
   GlobalConfig* GetGlobalConfig() { return m_globalConfig; }
   const GlobalConfig* GetGlobalConfig() const { return m_globalConfig; }
   void Setup(const std::string& globalConfigFileName = "", const std::string& tableFilename = "", const std::string& romName = "");
   void Init();
   void Finish();
   void MainThreadSignal();
   bool IsMainThreadActive() const { return m_mainThread.joinable(); }
   void ReceiveData(char type, int number, int value);
   void ReceiveData(const std::string& tableElementName, int value);
   void ReceiveData(const TableElementData& tableElementData);

private:
   void SetAlarms(AlarmHandler* alarms) { m_alarms = alarms; }
   void SetGlobalConfig(GlobalConfig* globalConfig) { m_globalConfig = globalConfig; }

   void InitMainThread();
   void FinishMainThread();
   void MainThreadDoIt();

   Cabinet* m_cabinet;
   Table* m_table;
   AlarmHandler* m_alarms;
   GlobalConfig* m_globalConfig;
   InputQueue* m_inputQueue;

   std::thread m_mainThread;
   std::mutex m_mainThreadMutex;
   std::condition_variable m_mainThreadCV;
   std::atomic<bool> m_keepMainThreadAlive;
   std::atomic<bool> m_mainThreadDoWork;

   static const int MAX_INPUT_DATA_PROCESSING_TIME_MS = 10;
};

}
