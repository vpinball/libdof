#include "NamedPipeServer.h"
#include "../../../general/StringExtensions.h"
#include <thread>
#include <vector>
#include <stdexcept>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#endif

namespace DOF
{

const std::string NamedPipeServer::s_pipeName = "ComPortServerPipe";

NamedPipeServer::NamedPipeServer(const std::string& comPort)
   : m_comPort(comPort)
{
   if (sp_get_port_by_name(comPort.c_str(), &m_serialPort) != SP_OK || !m_serialPort)
   {
      m_serialPort = nullptr;
      throw std::runtime_error(StringExtensions::Build("Serial port {0} not found", comPort));
   }

   if (sp_open(m_serialPort, SP_MODE_READ_WRITE) != SP_OK)
   {
      sp_free_port(m_serialPort);
      m_serialPort = nullptr;
      throw std::runtime_error(StringExtensions::Build("Could not open serial port {0}", comPort));
   }

   sp_set_baudrate(m_serialPort, 2000000);
   sp_set_bits(m_serialPort, 8);
   sp_set_parity(m_serialPort, SP_PARITY_NONE);
   sp_set_stopbits(m_serialPort, 1);
   sp_set_rts(m_serialPort, SP_RTS_ON);
   sp_set_dtr(m_serialPort, SP_DTR_ON);
}

NamedPipeServer::~NamedPipeServer() { StopServer(); }

void NamedPipeServer::StartServer()
{
   m_serverThread = std::thread(
      [this]()
      {
         while (m_isRunning)
         {
#ifdef _WIN32
            std::string pipePath = "\\\\.\\pipe\\" + s_pipeName;
            HANDLE serverPipe = CreateNamedPipeA(pipePath.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 1024, 1024, 0, nullptr);

            if (serverPipe == INVALID_HANDLE_VALUE)
            {
               continue;
            }

            if (ConnectNamedPipe(serverPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED)
            {
               HandleClientConnection(serverPipe);
            }

            CloseHandle(serverPipe);
#else
            int serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (serverSock < 0)
            {
               continue;
            }

            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            std::string sockPath = "/tmp/" + s_pipeName;
            strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);

            unlink(sockPath.c_str());

            if (bind(serverSock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            {
               close(serverSock);
               continue;
            }

            if (listen(serverSock, 1) < 0)
            {
               close(serverSock);
               continue;
            }

            int clientSock = -1;
            while (m_isRunning)
            {
               fd_set readFds;
               FD_ZERO(&readFds);
               FD_SET(serverSock, &readFds);
               struct timeval timeout;
               timeout.tv_sec = 0;
               timeout.tv_usec = 200000;
               int selectResult = select(serverSock + 1, &readFds, nullptr, nullptr, &timeout);
               if (selectResult > 0)
               {
                  clientSock = accept(serverSock, nullptr, nullptr);
                  break;
               }
               if (selectResult < 0)
                  break;
            }

            if (clientSock >= 0)
            {
               HandleClientConnection(reinterpret_cast<void*>(static_cast<intptr_t>(clientSock)));
               close(clientSock);
            }

            close(serverSock);
            unlink(sockPath.c_str());
#endif
         }
      });
}

void NamedPipeServer::HandleClientConnection(void* serverStream)
{
   bool completed = false;

   while (m_isRunning && !completed)
   {
      try
      {
         std::vector<char> request(1024);
         int bytesRead = 0;

#ifdef _WIN32
         HANDLE pipe = static_cast<HANDLE>(serverStream);
         DWORD dwBytesRead;
         if (!ReadFile(pipe, request.data(), static_cast<DWORD>(request.size()), &dwBytesRead, nullptr))
         {
            break;
         }
         bytesRead = static_cast<int>(dwBytesRead);
#else
         int sock = static_cast<int>(reinterpret_cast<intptr_t>(serverStream));
         bool readReady = false;
         while (m_isRunning && !readReady)
         {
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET(sock, &readFds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000;
            int selectResult = select(sock + 1, &readFds, nullptr, nullptr, &timeout);
            if (selectResult > 0)
               readReady = true;
            else if (selectResult < 0)
               break;
         }
         if (!readReady)
         {
            break;
         }

         bytesRead = static_cast<int>(read(sock, request.data(), request.size()));
         if (bytesRead <= 0)
         {
            break;
         }
#endif

         std::string requestStr(request.data(), bytesRead);

         if (StringExtensions::StartsWith(requestStr, "CONNECT"))
         {
            if (m_serialPort && sp_get_port_handle(m_serialPort, nullptr) == SP_OK)
            {
               sp_open(m_serialPort, SP_MODE_READ_WRITE);
            }
            std::string response = "OK";
#ifdef _WIN32
            HANDLE pipe = static_cast<HANDLE>(serverStream);
            DWORD bytesWritten;
            WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, nullptr);
#else
            int sock = static_cast<int>(reinterpret_cast<intptr_t>(serverStream));
            ssize_t bytesWritten = write(sock, response.c_str(), response.length());
            (void)bytesWritten;
#endif
         }
         else if (StringExtensions::StartsWith(requestStr, "STOP_SERVER"))
         {
            m_isRunning = false;
         }
         else if (StringExtensions::StartsWith(requestStr, "DISCONNECT"))
         {
            completed = true;
         }
         else if (StringExtensions::StartsWith(requestStr, "WRITE"))
         {
            std::string base64Data = requestStr.substr(6);
            std::vector<uint8_t> bytesToWrite = StringExtensions::FromBase64(base64Data);

            if (m_serialPort)
            {
               sp_blocking_write(m_serialPort, bytesToWrite.data(), bytesToWrite.size(), 500);
            }

            std::string response = "OK";
#ifdef _WIN32
            HANDLE pipe = static_cast<HANDLE>(serverStream);
            DWORD bytesWritten;
            WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, nullptr);
#else
            int sock = static_cast<int>(reinterpret_cast<intptr_t>(serverStream));
            ssize_t bytesWritten = write(sock, response.c_str(), response.length());
            (void)bytesWritten;
#endif
         }
         else if (StringExtensions::StartsWith(requestStr, "READLINE"))
         {
            std::string response;
            if (m_serialPort)
            {
               char buffer[256];
               int bytesRead = sp_blocking_read(m_serialPort, buffer, sizeof(buffer) - 1, 500);
               if (bytesRead > 0)
               {
                  buffer[bytesRead] = '\0';
                  response = buffer;
               }
            }

#ifdef _WIN32
            HANDLE pipe = static_cast<HANDLE>(serverStream);
            DWORD bytesWritten;
            WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, nullptr);
#else
            int sock = static_cast<int>(reinterpret_cast<intptr_t>(serverStream));
            ssize_t bytesWritten = write(sock, response.c_str(), response.length());
            (void)bytesWritten;
#endif
         }
         else if (StringExtensions::StartsWith(requestStr, "CHECK"))
         {
            std::string response = "FALSE";
            if (m_serialPort)
            {
               void* handle;
               if (sp_get_port_handle(m_serialPort, &handle) == SP_OK && handle != nullptr)
               {
                  response = "TRUE";
               }
            }

#ifdef _WIN32
            HANDLE pipe = static_cast<HANDLE>(serverStream);
            DWORD bytesWritten;
            WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, nullptr);
#else
            int sock = static_cast<int>(reinterpret_cast<intptr_t>(serverStream));
            ssize_t bytesWritten = write(sock, response.c_str(), response.length());
            (void)bytesWritten;
#endif
         }
         else if (StringExtensions::StartsWith(requestStr, "COMPORT"))
         {
#ifdef _WIN32
            HANDLE pipe = static_cast<HANDLE>(serverStream);
            DWORD bytesWritten;
            WriteFile(pipe, m_comPort.c_str(), static_cast<DWORD>(m_comPort.length()), &bytesWritten, nullptr);
#else
            int sock = static_cast<int>(reinterpret_cast<intptr_t>(serverStream));
            ssize_t bytesWritten = write(sock, m_comPort.c_str(), m_comPort.length());
            (void)bytesWritten;
#endif
         }
      }
      catch (...)
      {
         completed = true;
      }
   }
}

void NamedPipeServer::StopServer()
{
   m_isRunning = false;
   if (m_serverThread.joinable())
   {
      m_serverThread.join();
   }

   if (m_serialPort)
   {
      sp_close(m_serialPort);
      sp_free_port(m_serialPort);
      m_serialPort = nullptr;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

}