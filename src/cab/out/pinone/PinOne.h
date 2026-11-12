#pragma once

#include "../OutputControllerFlexCompleteBase.h"
#include <string>
#include <mutex>
#include <vector>
#include <cstdint>
#include <chrono>

namespace DOF
{

class Cabinet;
class PinOneCommunication;
class PinOneHidDevice;

class PinOne : public OutputControllerFlexCompleteBase
{
public:
   static const std::string HidSentinel;

private:
   std::mutex m_numberUpdateLocker;
   int m_number = -1;

   int m_minCommandIntervalMs = 1;
   bool m_minCommandIntervalMsSet = false;

   std::string m_comPort = "comm1";
   bool m_comPortSet = false;
   std::recursive_mutex m_portLocker;
   PinOneCommunication* m_pinOneCommunication = nullptr;
   PinOneHidDevice* m_hidDevice = nullptr;
   std::string m_hidDevicePath;

   std::vector<uint8_t> m_oldOutputValues;
   std::chrono::steady_clock::time_point m_lastUpdate;

   void UpdateDelay();

public:
   PinOne();
   PinOne(const std::string& comPort);
   virtual ~PinOne();

   int GetNumber() const { return m_number; }
   void SetNumber(int value);

   int GetMinCommandIntervalMs() const { return m_minCommandIntervalMs; }
   void SetMinCommandIntervalMs(int value);

   const std::string& GetComPort() const { return m_comPort; }
   void SetComPort(const std::string& value);

   virtual void Init(Cabinet* cabinet) override;
   virtual void Finish() override;

   virtual std::string GetXmlElementName() const override { return "PinOne"; }
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;

protected:
   virtual bool VerifySettings() override;
   virtual void UpdateOutputs(const std::vector<uint8_t>& newOutputValues) override;
   virtual void ConnectToController() override;
   virtual void DisconnectFromController() override;
};

}