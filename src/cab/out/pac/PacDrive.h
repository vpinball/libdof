#pragma once

#include "../OutputControllerBase.h"
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace DOF
{

class Cabinet;

class PacDrive : public OutputControllerBase
{
public:
   PacDrive();
   virtual ~PacDrive();

   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;
   virtual void Update() override;

   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;
   virtual std::string GetXmlElementName() const override { return "PacDrive"; }

protected:
   virtual void OnOutputValueChanged(IOutput* output) override;

private:
   class PacDriveUnit;
   static PacDriveUnit* s_pacDriveInstance;

   void AddOutputs();

   class PacDriveUnit
   {
   public:
      PacDriveUnit();
      ~PacDriveUnit();

      void Init(Cabinet* cabinet);
      void Finish();
      void UpdateValue(IOutput* output);
      void TriggerPacDriveUpdaterThread();
      bool ShutdownLighting();

      bool GetUpdateRequired() const { return m_updateRequired; }

   private:
      static const int MAX_UPDATE_FAIL_COUNT = 5;

      uint16_t m_newValue;
      uint16_t m_currentValue;
      std::atomic<bool> m_updateRequired;
      int m_index;

      std::mutex m_pacDriveUpdateLocker;
      std::mutex m_valueChangeLocker;

      std::thread m_pacDriveUpdater;
      std::atomic<bool> m_keepPacDriveUpdaterAlive;
      std::mutex m_pacDriveUpdaterThreadLocker;
      std::condition_variable m_updateCondition;
      std::atomic<bool> m_triggerUpdate;
      std::atomic<bool> m_updaterThreadFinished { false };

      void StartPacDriveUpdaterThread();
      void TerminatePacDriveUpdaterThread();
      void PacDriveUpdaterDoIt();
      void SendPacDriveUpdate();
      void CopyNewToCurrent();
      bool IsUpdaterThreadAlive() const;
      bool PacDriveSetLEDStates(uint16_t data);
   };
};

}