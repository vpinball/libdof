#include "NamedPipeServer.h"
#include "../../../general/StringExtensions.h"
#include "../../../Log.h"
#include <thread>
#include <vector>
#include <stdexcept>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#endif

namespace DOF
{

const std::string NamedPipeServer::s_pipeName = "ComPortServerPipe";

NamedPipeServer::NamedPipeServer(const std::string& comPort)
   : m_comPort(comPort)
{
   // sp_get_port_by_name does NOT validate the device on Linux (it only copies
   // the name; fd stays -1), so m_serialPort is non-null for any string. The
   // real check is sp_open's return - previously ignored, so a bad/default port
   // name (the class default "comm1", or a Windows "COMx") failed silently and
   // every subsequent write vanished into a dead fd (root cause of vpinball #2962).
   if (sp_get_port_by_name(comPort.c_str(), &m_serialPort) != SP_OK || !m_serialPort)
   {
      Log::Warning(StringExtensions::Build(
         "PinOne: serial port '{0}' not found (on Linux use /dev/ttyACM0, not COMx/comm1).", comPort));
      m_serialPort = nullptr;
      return;
   }

   enum sp_return openResult = sp_open(m_serialPort, SP_MODE_READ_WRITE);
   if (openResult != SP_OK)
   {
      char* errorMessage = sp_last_error_message();
      Log::Warning(StringExtensions::Build(
         "PinOne: could not open serial port '{0}' (rc={1}): {2}", comPort,
         std::to_string(static_cast<int>(openResult)), errorMessage ? errorMessage : "unknown error"));
      if (errorMessage)
         sp_free_error_message(errorMessage);
      sp_free_port(m_serialPort);
      m_serialPort = nullptr;   // so the if(m_serialPort) write-guard correctly skips
      return;
   }

   sp_set_baudrate(m_serialPort, 2000000);
   sp_set_bits(m_serialPort, 8);
   sp_set_parity(m_serialPort, SP_PARITY_NONE);
   sp_set_stopbits(m_serialPort, 1);
   sp_set_rts(m_serialPort, SP_RTS_ON);
   sp_set_dtr(m_serialPort, SP_DTR_ON);
}

NamedPipeServer::~NamedPipeServer()
{
   StopServer();
   if (m_serialPort)
   {
      sp_close(m_serialPort);
      sp_free_port(m_serialPort);
   }
}

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

            // Bound the accept() wait so the loop periodically re-checks
            // m_isRunning; otherwise StopServer()'s join() hangs forever on a
            // server thread blocked in accept() with no incoming client.
            struct timeval acceptTimeout;
            acceptTimeout.tv_sec = 0;
            acceptTimeout.tv_usec = 200000;
            setsockopt(serverSock, SOL_SOCKET, SO_RCVTIMEO, &acceptTimeout, sizeof(acceptTimeout));

            int clientSock = accept(serverSock, nullptr, nullptr);
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
               int written = sp_blocking_write(m_serialPort, bytesToWrite.data(), bytesToWrite.size(), 500);
               if (written < static_cast<int>(bytesToWrite.size()))
                  Log::Warning(StringExtensions::Build(
                     "PinOne: serial write short/failed ({0} of {1} bytes)",
                     std::to_string(written), std::to_string(static_cast<int>(bytesToWrite.size()))));
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
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

}