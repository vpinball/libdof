#pragma once

#include "../OutputControllerCompleteBase.h"
#include <string>
#include <mutex>
#include <libserialport.h>

namespace DOF
{

class Cabinet;

class PinControl : public OutputControllerCompleteBase
{
public:
   PinControl();
   virtual ~PinControl();

   const std::string& GetComPort() const { return m_comPort; }
   void SetComPort(const std::string& value) { m_comPort = value; }

   virtual std::string GetXmlElementName() const override { return "PinControl"; }
   virtual tinyxml2::XMLElement* ToXml(tinyxml2::XMLDocument& doc) const override;
   virtual bool FromXml(const tinyxml2::XMLElement* element) override;

protected:
   virtual int GetNumberOfConfiguredOutputs() override;
   virtual bool VerifySettings() override;
   virtual void UpdateOutputs(const std::vector<uint8_t>& outputValues) override;
   virtual void ConnectToController() override;
   virtual void DisconnectFromController() override;

private:
   std::string m_comPort;
   uint8_t* m_oldValues;
   std::recursive_mutex m_portLocker;
   struct sp_port* m_port;
};

}