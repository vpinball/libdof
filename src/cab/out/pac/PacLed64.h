#pragma once

#include "../OutputControllerBase.h"
#include "../IOutputController.h"
#include <hidapi/hidapi.h>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

namespace DOF
{

class Cabinet;

class PacLed64 : public OutputControllerBase
{
public:
   PacLed64();
   PacLed64(int id);
   virtual ~PacLed64();

   int GetId() const { return m_id; }
   void SetId(int value);

   int GetMinUpdateIntervalMs() const { return m_minUpdateIntervalMs; }
   void SetMinUpdateIntervalMs(int value);

   int GetFullUpdateThreshold() const { return m_fullUpdateThreshold; }
   void SetFullUpdateThreshold(int value);

   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;
   virtual void Update() override;

   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "PacLed64"; }

protected:
   virtual void OnOutputValueChanged(IOutput* output) override;

private:
   class PacLed64Unit;

   std::mutex m_idUpdateLocker;
   int m_id;
   bool m_minUpdateIntervalMsSet;
   int m_minUpdateIntervalMs;
   int m_fullUpdateThreshold;

   static std::map<int, PacLed64Unit*> s_pacLed64Units;
   static bool s_unitsInitialized;
   static void InitializeUnits();

   void AddOutputs();

   class PacLed64Unit
   {
   public:
      PacLed64Unit(int id);
      ~PacLed64Unit();

      int GetId() const { return m_id; }
      void SetFullUpdateThreshold(int value) { m_fullUpdateThreshold = value; }
      void SetMinUpdateInterval(std::chrono::milliseconds interval) { m_minUpdateInterval = interval; }

      void Init(Cabinet* cabinet);
      void Finish();
      void UpdateValue(IOutput* output);
      void TriggerPacLed64UpdaterThread();
      bool IsPresent() const;
      void ShutdownLighting();

      bool GetUpdateRequired() const { return m_updateRequired; }

   private:
      static const int MAX_UPDATE_FAIL_COUNT = 5;

      int m_id;
      int m_fullUpdateThreshold;
      std::chrono::milliseconds m_minUpdateInterval;
      std::chrono::steady_clock::time_point m_lastCommand;
      int m_index;

      std::vector<uint8_t> m_newValue;
      std::vector<uint8_t> m_currentValue;
      std::vector<uint8_t> m_lastValueSent;
      std::vector<bool> m_lastStateSent;

      std::atomic<bool> m_updateRequired;
      std::mutex m_pacLed64UpdateLocker;
      std::mutex m_valueChangeLocker;

      std::thread m_pacLed64Updater;
      std::atomic<bool> m_keepPacLed64UpdaterAlive;
      std::mutex m_pacLed64UpdaterThreadLocker;
      std::condition_variable m_updateCondition;
      std::atomic<bool> m_triggerUpdate;


      void StartPacLed64UpdaterThread();
      void TerminatePacLed64UpdaterThread();
      void PacLed64UpdaterDoIt();
      void SendPacLed64Update();
      void CopyNewToCurrent();
      void ResetFadeTime();
      void EnforceMinUpdateInterval();
      void InitUnit();
      bool IsUpdaterThreadAlive() const;

      std::atomic<bool> m_forceFullUpdate;
      std::atomic<bool> m_updaterThreadFinished { false };
   };
};

}