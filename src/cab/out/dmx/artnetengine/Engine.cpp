#include "Engine.h"
#include "../../../../Log.h"
#include "../../../../general/StringExtensions.h"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace DOF
{

Engine* Engine::s_instance = nullptr;
std::mutex Engine::s_instanceMutex;

Engine* Engine::GetInstance()
{
   std::lock_guard<std::mutex> lock(s_instanceMutex);
   if (s_instance == nullptr)
   {
      s_instance = new Engine();
   }
   return s_instance;
}

Engine::Engine()
   : m_socket(-1)
   , m_socketInitialized(false)
   , m_sendExceptionCount(0)
{
   m_artNetHeader = { 0x41, 0x72, 0x74, 0x2d, 0x4e, 0x65, 0x74, 0 };

#ifdef _WIN32
   WSADATA wsaData;
   if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
   {
      Log::Exception("Could not initialize Winsock for ArtNet");
      return;
   }
#endif

   std::lock_guard<std::mutex> lock(m_udpMutex);
   InitializeSocket();
}

Engine::~Engine()
{
   CloseSocket();

#ifdef _WIN32
   WSACleanup();
#endif
}

bool Engine::InitializeSocket()
{
   try
   {
      m_socket = socket(AF_INET, SOCK_DGRAM, 0);
      if (m_socket < 0)
      {
         Log::Exception("Could not create UDP socket for ArtNet");
         return false;
      }

      int broadcast = 1;
      if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) < 0)
      {
         Log::Exception("Could not set broadcast option for ArtNet socket");
         CloseSocket();
         return false;
      }

      int reuseaddr = 1;
      if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr)) < 0)
      {
         Log::Exception("Could not set reuse address option for ArtNet socket");
         CloseSocket();
         return false;
      }

#ifdef _WIN32
      DWORD timeout = 100;
      if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0)
      {
         Log::Exception("Could not set send timeout for ArtNet socket");
      }
#else
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
      {
         Log::Exception("Could not set send timeout for ArtNet socket");
      }
#endif

      m_socketInitialized = true;
      return true;
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Could not initialize UDP socket for ArtNet: {0}", e.what()));
      return false;
   }
}

void Engine::CloseSocket()
{
   if (m_socket >= 0)
   {
#ifdef _WIN32
      closesocket(m_socket);
#else
      close(m_socket);
#endif
      m_socket = -1;
      m_socketInitialized = false;
   }
}

bool Engine::Reopen()
{
   std::lock_guard<std::mutex> lock(m_udpMutex);
   if (m_socketInitialized && m_socket >= 0)
      return true;
   m_sendExceptionCount = 0;
   return InitializeSocket();
}

uint8_t Engine::LoByte(int value) const { return static_cast<uint8_t>(value & 0xFF); }

uint8_t Engine::HiByte(int value) const { return static_cast<uint8_t>((value >> 8) & 0xFF); }

bool Engine::SendDMX(const std::string& broadcastAddress, short universe, const std::vector<uint8_t>& data, int dataLength)
{
   std::lock_guard<std::mutex> lock(m_udpMutex);

   if (!m_socketInitialized || m_socket < 0)
   {
      Log::Exception("ArtNet engine socket not initialized");
      return false;
   }

   std::vector<uint8_t> packet(0x12 + dataLength);

   std::memcpy(packet.data(), m_artNetHeader.data(), m_artNetHeader.size());

   packet[8] = LoByte(0x5000);
   packet[9] = HiByte(0x5000);

   packet[10] = 0;
   packet[11] = 14;

   packet[12] = 0;
   packet[13] = 0;

   packet[14] = LoByte(universe);
   packet[15] = HiByte(universe);

   packet[16] = HiByte(dataLength);
   packet[17] = LoByte(dataLength);

   try
   {
      std::memcpy(packet.data() + 0x12, data.data(), dataLength);
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Exception occurred copying DMX data in ArtNet Engine: {0}", e.what()));
      return false;
   }

   if (m_socketInitialized && m_socket >= 0)
   {
      try
      {
         struct sockaddr_in addr;
         std::memset(&addr, 0, sizeof(addr));
         addr.sin_family = AF_INET;
         addr.sin_port = htons(ARTNET_PORT);

         if (broadcastAddress.empty() || broadcastAddress == "255.255.255.255")
         {
            addr.sin_addr.s_addr = INADDR_BROADCAST;
         }
         else
         {
            if (inet_pton(AF_INET, broadcastAddress.c_str(), &addr.sin_addr) <= 0)
            {
               Log::Exception(StringExtensions::Build("Invalid broadcast address for ArtNet: {0}", broadcastAddress));
               return false;
            }
         }

#ifdef _WIN32
         int result = sendto(m_socket, (const char*)packet.data(), static_cast<int>(packet.size()), 0, (struct sockaddr*)&addr, sizeof(addr));
#else
         ssize_t result = sendto(m_socket, (const char*)packet.data(), packet.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
#endif

         if (result < 0)
         {
#ifdef _WIN32
            int error = WSAGetLastError();
            Log::Exception(StringExtensions::Build("Failed to send ArtNet packet, error: {0}", std::to_string(error)));
#else
            Log::Exception(StringExtensions::Build("Failed to send ArtNet packet, error: {0}", std::to_string(errno)));
#endif
            throw std::runtime_error("Send failed");
         }

         m_sendExceptionCount = 0;
         return true;
      }
      catch (const std::exception& e)
      {
         m_sendExceptionCount++;

         if (m_sendExceptionCount > MAX_CONSECUTIVE_FAILURES)
         {
            Log::Exception(StringExtensions::Build(
               "More than {0} consecutive transmissions of ArtNet data have failed. ArtNet engine will be disabled for the session.", std::to_string(MAX_CONSECUTIVE_FAILURES)));
            CloseSocket();
         }
         else
         {
            Log::Exception(StringExtensions::Build("Exception occurred when sending ArtNet data: {0}", e.what()));
         }
      }
   }
   return false;
}

}