#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace DOF
{

class NamedPipeServer;

class PinOneCommunication
{
private:
   void* m_pipeClient = nullptr;
   NamedPipeServer* m_server = nullptr;
   std::string m_pipeName = "ComPortServerPipe";
   std::string m_comPort;


public:
   PinOneCommunication(const std::string& comPort);
   ~PinOneCommunication();

   bool ConnectToServer();
   bool DisconnectFromServer();
   bool CreateServer();
   bool IsComPortConnected();
   void Disconnect();
   void Write(const std::vector<uint8_t>& bytesToWrite);
   std::string ReadLine();
   std::string GetCOMPort();

private:
   void CloseClient();
   void SendPipeMessage(const std::string& message);
   std::string ReadMessage();
};

}