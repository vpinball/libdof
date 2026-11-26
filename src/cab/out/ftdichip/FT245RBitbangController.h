#pragma once

#include "../OutputControllerBase.h"
#include "../IOutputController.h"
#include "FTDI.h"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace DOF
{

class Cabinet;

class FT245RBitbangController : public OutputControllerBase
{
public:
   FT245RBitbangController();
   virtual ~FT245RBitbangController();

   const std::string& GetSerialNumber() const { return m_serialNumber; }
   void SetSerialNumber(const std::string& value) { m_serialNumber = value; }

   const std::string& GetDescription() const { return m_description; }
   void SetDescription(const std::string& value) { m_description = value; }

   int GetId() const { return m_id; }
   void SetId(int value);

   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;
   virtual void Update() override;

   bool GetUpdaterThreadIsActive() const;

   virtual std::string GetXmlElementName() const override { return "FT245RBitbangController"; }
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;

protected:
   virtual void OnOutputValueChanged(IOutput* output) override;

private:
   std::string m_serialNumber;
   std::string m_description;
   int m_id;

   void AddOutputs();

   void InitUpdaterThread();
   void FinishUpdaterThread();
   void UpdaterThreadSignal();
   void UpdaterThreadDoIt();

   std::mutex m_valueChangeLocker;
   std::atomic<uint8_t> m_newValue;
   std::atomic<uint8_t> m_currentValue;

   std::thread* m_updaterThread;
   std::mutex m_updaterThreadLocker;
   std::condition_variable m_updaterThreadCondition;
   std::atomic<bool> m_keepUpdaterThreadAlive;
   std::atomic<bool> m_updaterThreadFinished;
   std::string m_finishError;
   int m_firstTryFailCnt;

   std::recursive_mutex m_ftdiLocker;
   FTDI* m_ftdi;

   void Connect();
   void SendUpdate(uint8_t outputData);
   void Disconnect();
};

}