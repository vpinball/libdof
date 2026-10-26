#include "PinOneCommunication.h"
#include "NamedPipeServer.h"
#include "../../../general/StringExtensions.h"
#include <cstring>
#include <vector>
#include <stdexcept>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#endif

namespace DOF
{

PinOneCommunication::PinOneCommunication(const std::string& comPort)
   : m_comPort(comPort)
{
}

PinOneCommunication::~PinOneCommunication()
{
   try
   {
      if (m_pipeClient)
      {
#ifdef _WIN32
         CloseHandle(static_cast<HANDLE>(m_pipeClient));
#else
         close(static_cast<int>(reinterpret_cast<intptr_t>(m_pipeClient)));
#endif
         m_pipeClient = nullptr;
      }
      if (m_server)
      {
         delete m_server;
         m_server = nullptr;
      }
   }
   catch (...)
   {
   }
}

bool PinOneCommunication::ConnectToServer()
{
   try
   {
#ifdef _WIN32
      std::string pipePath = "\\\\.\\pipe\\" + m_pipeName;
      HANDLE pipe = CreateFileA(pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

      if (pipe == INVALID_HANDLE_VALUE)
         return false;

      DWORD mode = PIPE_READMODE_BYTE;
      if (!SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr))
      {
         CloseHandle(pipe);
         return false;
      }

      m_pipeClient = pipe;
#else
      int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
      if (sockfd < 0)
         return false;

      // Set socket to non-blocking for timeout
      int flags = fcntl(sockfd, F_GETFL, 0);
      fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

      struct sockaddr_un addr;
      memset(&addr, 0, sizeof(addr));
      addr.sun_family = AF_UNIX;
      strncpy(addr.sun_path, ("/tmp/" + m_pipeName).c_str(), sizeof(addr.sun_path) - 1);

      if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
      {
         if (errno == EINPROGRESS)
         {
            // Wait for connection with timeout
            fd_set writefds;
            FD_ZERO(&writefds);
            FD_SET(sockfd, &writefds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms timeout to match C# version

            int result = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
            if (result <= 0)
            {
               close(sockfd);
               return false;
            }
         }
         else
         {
            // Connection failed immediately (socket doesn't exist)
            close(sockfd);
            return false;
         }
      }

      // Restore blocking mode
      fcntl(sockfd, F_SETFL, flags);

      m_pipeClient = reinterpret_cast<void*>(static_cast<intptr_t>(sockfd));
#endif
      return true;
   }
   catch (...)
   {
      return false;
   }
}

bool PinOneCommunication::DisconnectFromServer()
{
   Disconnect();
   if (m_server)
   {
      m_server->StopServer();
      return true;
   }
   return false;
}

bool PinOneCommunication::CreateServer()
{
   try
   {
      if (!StringExtensions::IsNullOrEmpty(m_comPort))
      {
         m_server = new NamedPipeServer(m_comPort);
         m_server->StartServer();
         std::this_thread::sleep_for(std::chrono::milliseconds(300));
         return true;
      }
   }
   catch (...)
   {
      return false;
   }
   return false;
}

bool PinOneCommunication::IsComPortConnected()
{
   SendPipeMessage("CHECK");
   std::string response = ReadMessage();
   return response == "TRUE";
}

void PinOneCommunication::Disconnect() { SendPipeMessage("DISCONNECT"); }

void PinOneCommunication::Write(const std::vector<uint8_t>& bytesToWrite)
{
   std::string base64Bytes = StringExtensions::ToBase64(bytesToWrite);
   SendPipeMessage("WRITE " + base64Bytes);
   ReadMessage();
}

std::string PinOneCommunication::ReadLine()
{
   SendPipeMessage("READLINE");
   return ReadMessage();
}

std::string PinOneCommunication::GetCOMPort()
{
   SendPipeMessage("COMPORT");
   return ReadMessage();
}

void PinOneCommunication::SendPipeMessage(const std::string& message)
{
   try
   {
#ifdef _WIN32
      HANDLE pipe = static_cast<HANDLE>(m_pipeClient);
      DWORD bytesWritten;
      if (!WriteFile(pipe, message.c_str(), static_cast<DWORD>(message.length()), &bytesWritten, nullptr))
      {
         throw std::runtime_error("Failed to write to pipe");
      }
#else
      int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(m_pipeClient));
      ssize_t result = write(sockfd, message.c_str(), message.length());
      if (result < 0)
         throw std::runtime_error("Failed to write to socket");
#endif
   }
   catch (...)
   {
      if (CreateServer() && ConnectToServer())
      {
#ifdef _WIN32
         HANDLE pipe = static_cast<HANDLE>(m_pipeClient);
         DWORD bytesWritten;
         WriteFile(pipe, message.c_str(), static_cast<DWORD>(message.length()), &bytesWritten, nullptr);
#else
         int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(m_pipeClient));
         ssize_t bytesWritten = write(sockfd, message.c_str(), message.length());
         (void)bytesWritten;
#endif
      }
      else
         throw std::runtime_error("Unable to connect to board");
   }
}

std::string PinOneCommunication::ReadMessage()
{
   try
   {
      std::vector<char> response(1024);

#ifdef _WIN32
      HANDLE pipe = static_cast<HANDLE>(m_pipeClient);
      DWORD bytesRead;
      if (!ReadFile(pipe, response.data(), static_cast<DWORD>(response.size()), &bytesRead, nullptr))
      {
         throw std::runtime_error("Failed to read from pipe");
      }
      return std::string(response.data(), bytesRead);
#else
      int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(m_pipeClient));
      ssize_t bytesRead = read(sockfd, response.data(), response.size());
      if (bytesRead < 0)
         throw std::runtime_error("Failed to read from socket");
      return std::string(response.data(), bytesRead);
#endif
   }
   catch (...)
   {
      if (CreateServer() && ConnectToServer())
         return ReadMessage();
      else
         return "";
   }
}

}