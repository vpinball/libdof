#pragma once

#include <string>
#include <atomic>
#include <thread>

#include <libserialport.h>

namespace DOF
{

class NamedPipeServer
{
private:
   std::atomic<bool> m_isRunning { true };
   std::atomic<bool> m_serverThreadDone { false };
   std::string m_comPort;
   static const std::string s_pipeName;
   std::thread m_serverThread;
   struct sp_port* m_serialPort = nullptr;

   void HandleClientConnection(void* serverStream);

public:
   NamedPipeServer(const std::string& comPort);
   ~NamedPipeServer();

   void StartServer();
   void StopServer();
};

}