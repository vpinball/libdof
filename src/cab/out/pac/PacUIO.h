#pragma once

#include "../OutputControllerBase.h"
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <tinyxml2/tinyxml2.h>

namespace DOF
{

class PacUIO : public OutputControllerBase
{
private:
   std::mutex m_idUpdateLocker;
   int m_id = -1;

public:
   int GetId() const { return m_id; }
   void SetId(int value);

   void Update() override;
   void Init(class Cabinet* cabinet) override;
   void Finish() override;

   tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   bool FromXml(const tinyxml2::XMLElement* element) override;
   std::string GetXmlElementName() const override { return "PacUIO"; }

private:
   void AddOutputs();
   void OnOutputValueChanged(class IOutput* output) override;

public:
   PacUIO();
   PacUIO(int id);
   ~PacUIO();

private:
   class PacUIOUnit
   {
   private:
      static const int MAX_UPDATE_FAIL_COUNT = 5;
      int m_id;
      int m_index = -1;
      class PacDriveSingleton* m_pdSingleton = nullptr;

      uint8_t m_newValue[96];
      uint8_t m_currentValue[96];
      uint8_t m_lastValueSent[96];
      bool m_lastStateSent[96];

      bool m_updateRequired = true;
      bool m_forceFullUpdate = false;

      std::mutex m_pacUIOUpdateLocker;
      std::mutex m_valueChangeLocker;

      std::thread m_pacUIOUpdater;
      std::atomic<bool> m_keepPacUIOUpdaterAlive { false };
      std::mutex m_pacUIOUpdaterThreadLocker;
      std::condition_variable m_triggerCondition;
      std::atomic<bool> m_triggerUpdate { false };
      std::atomic<bool> m_updaterThreadFinished { false };

   public:
      PacUIOUnit(int id);
      ~PacUIOUnit();

      void Init(class Cabinet* cabinet);
      void Finish();
      void UpdateValue(class IOutput* output);
      void TriggerPacUIOUpdaterThread();
      void ShutdownLighting();

      bool IsUpdaterThreadAlive();
      void StartPacUIOUpdaterThread();
      void TerminatePacUIOUpdaterThread();

   private:
      void CopyNewToCurrent();
      void PacUIOUpdaterDoIt();
      void SendPacUIOUpdate();
      void ResetFadeTime();
      void InitUnit();

      bool IsPresent();
      void OnPacAttached(int index);
      void OnPacRemoved(int index);
   };

   static std::map<int, PacUIOUnit*> s_pacUIOUnits;
   static bool s_unitsInitialized;
   static void InitializeUnits();
};

}