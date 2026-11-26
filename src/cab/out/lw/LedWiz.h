#pragma once

#include "../OutputControllerBase.h"
#include <hidapi/hidapi.h>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace DOF
{

class LedWizOutput;

class LedWiz : public OutputControllerBase
{
public:
   LedWiz();
   LedWiz(int number);
   virtual ~LedWiz();

   int GetNumber() const { return m_number; }
   void SetNumber(int value);
   int GetMinCommandIntervalMs() const { return m_minCommandIntervalMs; }
   void SetMinCommandIntervalMs(int value);
   virtual void Update() override;
   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "LedWiz"; }

   static std::vector<int> GetLedwizNumbers();
   static void ClearDevices();

protected:
   virtual void OnOutputValueChanged(IOutput* output) override;

private:
   void AddOutputs();

   struct LWDEVICE
   {
      LWDEVICE(int unitNo, const std::string& path, const std::string& productName)
         : unitNo(unitNo)
         , path(path)
         , productName(productName)
      {
      }

      int unitNo;
      std::string path;
      std::string productName;
   };

   static std::vector<LWDEVICE> s_deviceList;
   static const int LWZ_MAX_DEVICES = 16;

   enum class AutoPulseMode : int
   {
      RampUpRampDown = 129,
      OnOff = 130,
      OnRampDown = 131,
      RampUpDown = 132
   };

   static int s_startedUp;
   static std::mutex s_startupLocker;
   static void StartupLedWiz();
   static void TerminateLedWiz();
   static std::string GetDeviceProductName(hid_device_info* dev);
   static std::string GetDeviceManufacturerName(hid_device_info* dev);
   static int GetOutputReportByteLength(hid_device* dev);

   std::mutex m_numberUpdateLocker;
   int m_number;
   int m_minCommandIntervalMs;
   bool m_minCommandIntervalMsSet;

   class LedWizUnit;

   static std::map<int, LedWizUnit*> s_ledWizUnits;
   static void InitializeUnits();

   class LedWizUnit
   {
   public:
      LedWizUnit(int number);
      ~LedWizUnit();

      int GetNumber() const { return m_number; }
      void Init(Cabinet* cabinet, int minCommandIntervalMs);
      void Finish();
      void UpdateValue(LedWizOutput* ledWizOutput);
      void CopyNewToCurrent();
      bool IsUpdaterThreadAlive() const;
      void StartLedWizUpdaterThread();
      void TerminateLedWizUpdaterThread();
      void TriggerLedWizUpdaterThread();
      void ShutdownLighting();

   private:
      enum class InUseStates
      {
         Startup,
         ValueChanged,
         Running
      };

      static const uint8_t s_byteToLedWizValue[256];
      static const int MAX_UPDATE_FAIL_COUNT = 5;
      static const int s_retryWait[10];

      void LedWizUpdaterDoIt();
      void UpdateDelay();
      void SendLedWizUpdate();
      void SBA(const uint8_t* b, uint8_t globalFlashSpeed = 2);
      void PBA(const uint8_t* b);
      void WriteUSB(const uint8_t* buf, size_t length);
      bool IsPresent() const;

      int m_number;
      std::string m_path;
      hid_device* m_fp;

      std::array<uint8_t, 32> m_newOutputValues;
      std::array<uint8_t, 32> m_currentOutputValues;
      std::array<uint8_t, 4> m_newAfterValueSwitches;
      std::array<uint8_t, 4> m_newBeforeValueSwitches;
      std::array<uint8_t, 4> m_currentAfterValueSwitches;
      std::array<uint8_t, 4> m_currentBeforeValueSwitches;
      bool m_newValueUpdateRequired;
      bool m_newSwitchUpdateBeforeValueUpdateRequired;
      bool m_newSwitchUpdateAfterValueUpdateRequired;
      bool m_currentValueUpdateRequired;
      bool m_currentSwitchUpdateBeforeValueUpdateRequired;
      bool m_currentSwitchUpdateAfterValueUpdateRequired;
      bool m_updateRequired;

      std::mutex m_ledWizUpdateLocker;
      std::recursive_mutex m_valueChangeLocker;

      std::thread m_ledWizUpdater;
      std::atomic<bool> m_keepLedWizUpdaterAlive;
      std::mutex m_ledWizUpdaterThreadLocker;
      std::condition_variable m_updateCondition;
      std::atomic<bool> m_triggerUpdate;
      std::atomic<bool> m_updaterThreadFinished;

      std::atomic<InUseStates> m_inUseState;

      std::chrono::steady_clock::time_point m_lastUpdate;
      int m_minCommandIntervalMs;
      std::chrono::steady_clock::time_point m_pbaTime;
      bool m_writeFailed = false;
      bool m_shutdownFailed = false;

   public:
      bool ShutdownFailed() const { return m_shutdownFailed; }

   private:
      void ForceFullUpdate();
   };
};

}
