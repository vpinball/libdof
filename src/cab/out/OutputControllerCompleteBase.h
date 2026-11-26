#pragma once

#include "DOF/DOF.h"
#include "OutputControllerBase.h"
#include "ISupportsSetValues.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <thread>

namespace DOF
{

class OutputControllerCompleteBase : public OutputControllerBase, public ISupportsSetValues
{
public:
   OutputControllerCompleteBase();
   ~OutputControllerCompleteBase();

   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;
   void Update() override;


   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;


   virtual void SetValues(int firstOutput, const uint8_t* values, int valueCount) override;

protected:
   void SetupOutputs();
   void RenameOutputs();

   virtual int GetNumberOfConfiguredOutputs() = 0;


   virtual void OnOutputValueChanged(IOutput* output) override;


   void OnNameChanged();


   virtual bool VerifySettings() = 0;
   virtual void ConnectToController() = 0;
   virtual void DisconnectFromController() = 0;
   virtual void UpdateOutputs(const std::vector<uint8_t>& outputValues) = 0;
   bool m_forceFullTransfer = false;
   bool m_transferSkipped = false;
   bool m_disconnectTurnedOff = false;


   void InitUpdaterThread();
   void FinishUpdaterThread();
   void UpdaterThreadSignal();
   bool IsUpdaterThreadActive() const;


   std::vector<uint8_t> m_outputValues;
   mutable std::mutex m_valueChangeMutex;
   bool m_updateRequired;

   enum class InUseState
   {
      Startup,
      ValueChanged,
      Running
   };
   InUseState m_inUseState;

private:
   std::unique_ptr<std::thread> m_updaterThread;
   mutable std::mutex m_updaterThreadMutex;
   std::condition_variable m_updateCondition;
   mutable std::mutex m_conditionMutex;
   bool m_keepUpdaterThreadAlive;
   std::atomic<bool> m_updaterThreadFinished { false };
   std::string m_finishError;
   void UpdaterThreadDoIt();
   bool Transfer(const std::vector<uint8_t>& values, bool full);
};

}
