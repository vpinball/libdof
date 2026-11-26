#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace DOF
{

class Engine
{
public:
   static Engine* GetInstance();

   bool SendDMX(const std::string& broadcastAddress, short universe, const std::vector<uint8_t>& data, int dataLength);
   bool Reopen();

private:
   Engine();
   ~Engine();

   Engine(const Engine&) = delete;
   Engine& operator=(const Engine&) = delete;

   bool InitializeSocket();
   void CloseSocket();
   uint8_t LoByte(int value) const;
   uint8_t HiByte(int value) const;

   static Engine* s_instance;
   static std::mutex s_instanceMutex;

   std::mutex m_udpMutex;
   int m_socket;
   bool m_socketInitialized;
   int m_sendExceptionCount;

   std::vector<uint8_t> m_artNetHeader;

   static const int ARTNET_PORT = 0x1936; // 6454
   static const int MAX_CONSECUTIVE_FAILURES = 10;
};

}