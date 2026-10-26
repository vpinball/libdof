#include "TeensyStripController.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../../general/MathExtensions.h"
#include <algorithm>
#include <numeric>
#include <thread>
#include <chrono>

namespace DOF
{

TeensyStripController::TeensyStripController()
   : m_numberOfLedsPerStrip(10, 0)
   , m_comPortName("")
   , m_comPortBaudRate(9600)
   , m_comPortParity(Parity::None)
   , m_comPortDataBits(8)
   , m_comPortStopBits(StopBits::One)
   , m_comPortTimeOutMs(200)
   , m_comPortOpenWaitMs(50)
   , m_comPortHandshakeStartWaitMs(20)
   , m_comPortHandshakeEndWaitMs(50)
   , m_comPortDtrEnable(false)
   , m_comPort(nullptr)
   , m_numberOfLedsPerChannel(-1)
{
}

TeensyStripController::~TeensyStripController() { DisconnectFromController(); }

void TeensyStripController::SetNumberOfLedsStrip1(int value)
{
   m_numberOfLedsPerStrip[0] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip2(int value)
{
   m_numberOfLedsPerStrip[1] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip3(int value)
{
   m_numberOfLedsPerStrip[2] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip4(int value)
{
   m_numberOfLedsPerStrip[3] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip5(int value)
{
   m_numberOfLedsPerStrip[4] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip6(int value)
{
   m_numberOfLedsPerStrip[5] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip7(int value)
{
   m_numberOfLedsPerStrip[6] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip8(int value)
{
   m_numberOfLedsPerStrip[7] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip9(int value)
{
   m_numberOfLedsPerStrip[8] = value;
   SetupOutputs();
}

void TeensyStripController::SetNumberOfLedsStrip10(int value)
{
   m_numberOfLedsPerStrip[9] = value;
   SetupOutputs();
}

void TeensyStripController::SetComPortTimeOutMs(int value)
{
   if (MathExtensions::IsBetween(value, 1, 5000))
   {
      m_comPortTimeOutMs = value;
   }
   else
   {
      m_comPortTimeOutMs = 200;
      Log::Warning(
         StringExtensions::Build("The specified value {0} for the ComPortTimeOutMs is outside the valid range of 1 to 5000. Will use the default value of 200ms.", std::to_string(value)));
   }
}

void TeensyStripController::SetComPortOpenWaitMs(int value)
{
   if (MathExtensions::IsBetween(value, 50, 5000))
   {
      m_comPortOpenWaitMs = value;
   }
   else
   {
      m_comPortOpenWaitMs = 50;
      Log::Warning(
         StringExtensions::Build("The specified value {0} for the ComPortOpenWaitMs is outside the valid range of 50 to 5000. Will use the default value of 50ms.", std::to_string(value)));
   }
}

void TeensyStripController::SetComPortHandshakeStartWaitMs(int value)
{
   if (MathExtensions::IsBetween(value, 20, 500))
   {
      m_comPortHandshakeStartWaitMs = value;
   }
   else
   {
      m_comPortHandshakeStartWaitMs = 20;
      Log::Warning(StringExtensions::Build(
         "The specified value {0} for the ComPortHandshakeStartWaitMs is outside the valid range of 20 to 500. Will use the default value of 20ms.", std::to_string(value)));
   }
}

void TeensyStripController::SetComPortHandshakeEndWaitMs(int value)
{
   if (MathExtensions::IsBetween(value, 50, 500))
   {
      m_comPortHandshakeEndWaitMs = value;
   }
   else
   {
      m_comPortHandshakeEndWaitMs = 50;
      Log::Warning(StringExtensions::Build(
         "The specified value {0} for the ComPortHandshakeEndWaitMs is outside the valid range of 50 to 500. Will use the default value of 50ms.", std::to_string(value)));
   }
}

int TeensyStripController::GetNumberOfConfiguredOutputs() { return std::accumulate(m_numberOfLedsPerStrip.begin(), m_numberOfLedsPerStrip.end(), 0) * 3; }

bool TeensyStripController::VerifySettings()
{
   if (m_comPortName.empty())
   {
      Log::Warning("The ComPortName has not been specified");
      return false;
   }

   struct sp_port** ports;
   enum sp_return result = sp_list_ports(&ports);
   if (result != SP_OK)
   {
      Log::Warning(StringExtensions::Build("Could not enumerate serial ports: {0}", sp_last_error_message()));
      return false;
   }

   bool portFound = false;
   for (int i = 0; ports[i]; i++)
   {
      const char* portName = sp_get_port_name(ports[i]);
      if (portName && m_comPortName == portName)
      {
         portFound = true;
         break;
      }
   }

   if (!portFound)
   {
      struct sp_port* port = nullptr;
      if (sp_get_port_by_name(m_comPortName.c_str(), &port) == SP_OK && port)
      {
         sp_free_port(port);
         portFound = true;
      }
   }

   if (!portFound)
   {
      std::string availablePorts;
      for (int i = 0; ports[i]; i++)
      {
         if (i > 0)
            availablePorts += ", ";
         availablePorts += sp_get_port_name(ports[i]);
      }
      Log::Warning(StringExtensions::Build("The specified Com-Port {0} was not found. Available com-ports: {1}", m_comPortName, availablePorts));
      sp_free_port_list(ports);
      return false;
   }

   sp_free_port_list(ports);

   for (int nr : m_numberOfLedsPerStrip)
   {
      if (nr < 0)
      {
         Log::Warning("At least one ledstrip has a invalid number of leds specified (<0).");
         return false;
      }
   }
   return true;
}

void TeensyStripController::SendLedstripData(const std::vector<uint8_t>& outputValues, int targetPosition)
{
   int nrOfLeds = static_cast<int>(outputValues.size()) / 3;
   std::vector<uint8_t> commandData = { (uint8_t)'R', (uint8_t)(targetPosition >> 8), (uint8_t)(targetPosition & 255), (uint8_t)(nrOfLeds >> 8), (uint8_t)(nrOfLeds & 255) };

   enum sp_return result = sp_blocking_write(m_comPort, commandData.data(), 5, m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write command data: {0}", sp_last_error_message()));

   result = sp_blocking_write(m_comPort, outputValues.data(), outputValues.size(), m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write output values: {0}", sp_last_error_message()));
}

void TeensyStripController::UpdateOutputs(const std::vector<uint8_t>& outputValues)
{
   if (!m_comPort)
      throw std::runtime_error("Comport is not initialized");

   std::vector<uint8_t> commandData;
   std::vector<uint8_t> answerData;
   int bytesRead;
   int sourcePosition = 0;

   for (int i = 0; i < 10; i++)
   {
      int nrOfLedsOnStrip = m_numberOfLedsPerStrip[i];
      if (nrOfLedsOnStrip > 0)
      {
         int targetPosition = i * m_numberOfLedsPerChannel;

         std::vector<uint8_t> stripData(outputValues.begin() + sourcePosition * 3, outputValues.begin() + sourcePosition * 3 + nrOfLedsOnStrip * 3);
         SendLedstripData(stripData, targetPosition);

         bytesRead = -1;
         answerData = std::vector<uint8_t>(1);

         try
         {
            bytesRead = ReadPortWait(answerData.data(), 0, 1);
         }
         catch (const std::exception& e)
         {
            throw std::runtime_error(
               StringExtensions::Build("A exception occurred while waiting for the ACK after sending the data for channel {0} of the {1}.", std::to_string(i + 1), GetXmlElementName()));
         }

         if (bytesRead != 1 || answerData[0] != (uint8_t)'A')
         {
            throw std::runtime_error(StringExtensions::Build(
               "Received no answer or a unexpected answer while waiting for the ACK after sending the data for channel {0} of the {1}.", std::to_string(i + 1), GetXmlElementName()));
         }
         sourcePosition += nrOfLedsOnStrip;
      }
   }

   commandData = { (uint8_t)'O' };
   enum sp_return result = sp_blocking_write(m_comPort, commandData.data(), 1, m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write output command: {0}", sp_last_error_message()));

   bytesRead = -1;
   answerData = std::vector<uint8_t>(1);

   try
   {
      bytesRead = ReadPortWait(answerData.data(), 0, 1);
   }
   catch (const std::exception& e)
   {
      throw std::runtime_error(StringExtensions::Build("A exception occurred while waiting for the ACK after sending the output command (O) to the {0}", GetXmlElementName()));
   }

   if (bytesRead != 1 || answerData[0] != (uint8_t)'A')
   {
      throw std::runtime_error(
         StringExtensions::Build("Received no answer or a unexpected answer while waiting for the ACK after sending the output command (O) to the {0}", GetXmlElementName()));
   }
}

void TeensyStripController::SetupController()
{
   uint8_t command = 'M';
   enum sp_return result = sp_blocking_write(m_comPort, &command, 1, m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write max leds query: {0}", sp_last_error_message()));

   std::vector<uint8_t> receiveData(3);
   int bytesRead = -1;

   try
   {
      bytesRead = ReadPortWait(receiveData.data(), 0, 3);
   }
   catch (const std::exception& e)
   {
      throw std::runtime_error(
         "Expected 3 bytes containing data on the max number of leds per channel, but the read operation resulted in a exception. Will not send data to the controller");
   }

   if (bytesRead != 3)
   {
      throw std::runtime_error(StringExtensions::Build(
         "The {0} did not send the expected 3 bytes containing the data on the max number of leds per channel. Received only {1} bytes. Will not send data to the controller",
         GetXmlElementName(), std::to_string(bytesRead)));
   }

   if (receiveData[2] != 'A')
   {
      throw std::runtime_error(
         StringExtensions::Build("The {0} did not send a ACK after the data containing the max number of leds per channel. Will not send data to the controller", GetXmlElementName()));
   }

   int maxNumberOfLedsPerChannel = receiveData[0] * 256 + receiveData[1];

   int maxLedsConfigured = *std::max_element(m_numberOfLedsPerStrip.begin(), m_numberOfLedsPerStrip.end());
   if (maxLedsConfigured > maxNumberOfLedsPerChannel)
   {
      throw std::runtime_error(
         StringExtensions::Build("The {0} boards supports up to {1} leds per channel, but you have defined up to {2} leds per channel. Will not send data to the controller.",
            GetXmlElementName(), std::to_string(maxNumberOfLedsPerChannel), std::to_string(maxLedsConfigured)));
   }

   m_numberOfLedsPerChannel = maxLedsConfigured;
   uint16_t nrOfLeds = (uint16_t)m_numberOfLedsPerChannel;
   std::vector<uint8_t> commandData = { (uint8_t)'L', (uint8_t)(nrOfLeds >> 8), (uint8_t)(nrOfLeds & 255) };

   result = sp_blocking_write(m_comPort, commandData.data(), 3, m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write leds per channel command: {0}", sp_last_error_message()));

   receiveData.resize(1);
   bytesRead = -1;

   try
   {
      bytesRead = ReadPortWait(receiveData.data(), 0, 1);
   }
   catch (const std::exception& e)
   {
      throw std::runtime_error("Expected 1 bytes after setting the number of leds per channel, but the read operation resulted in a exception. Will not send data to the controller.");
   }

   if (bytesRead != 1 || receiveData[0] != (uint8_t)'A')
   {
      throw std::runtime_error("Expected a Ack (A) after setting the number of leds per channel, but received no answer or a unexpected answer. Will not send data to the controller.");
   }
}

void TeensyStripController::ConnectToController()
{
   DisconnectFromController();

   struct sp_port** ports;
   enum sp_return result = sp_list_ports(&ports);
   if (result != SP_OK)
      throw std::runtime_error(StringExtensions::Build("Could not enumerate serial ports: {0}", sp_last_error_message()));

   bool portFound = false;
   std::string availablePorts;
   for (int i = 0; ports[i]; i++)
   {
      const char* portName = sp_get_port_name(ports[i]);
      if (i > 0)
         availablePorts += ", ";
      availablePorts += portName;

      if (portName && m_comPortName == portName)
         portFound = true;
   }

   if (!portFound)
   {
      struct sp_port* port = nullptr;
      if (sp_get_port_by_name(m_comPortName.c_str(), &port) == SP_OK && port)
      {
         sp_free_port(port);
         portFound = true;
      }
   }

   if (!portFound)
   {
      sp_free_port_list(ports);
      throw std::runtime_error(
         StringExtensions::Build("The specified Com-Port '{0}' does not exist. Found the following Com-Ports: {1}. Will not send data to the controller.", m_comPortName, availablePorts));
   }

   sp_free_port_list(ports);

   Log::Write(StringExtensions::Build("Initializing ComPort {0} with these settings :\n\tBaudRate {1}, Parity {2}, DataBits {3}, StopBits {4}, R/W Timeouts {5}ms\n\tHandshake Timings : "
                                      "Open {6}ms, Loop Start/End {7}/{8}ms, DTR enable {9}",
      std::vector<std::string> { m_comPortName, std::to_string(m_comPortBaudRate), std::to_string((int)m_comPortParity), std::to_string(m_comPortDataBits),
         std::to_string((int)m_comPortStopBits), std::to_string(m_comPortTimeOutMs), std::to_string(m_comPortOpenWaitMs), std::to_string(m_comPortHandshakeStartWaitMs),
         std::to_string(m_comPortHandshakeEndWaitMs), m_comPortDtrEnable ? "true" : "false" }));

   result = sp_get_port_by_name(m_comPortName.c_str(), &m_comPort);
   if (result != SP_OK)
      throw std::runtime_error(StringExtensions::Build("A exception occurred while setting the name of the Com-port '{0}'. Will not send data to the controller.", m_comPortName));

   result = sp_open(m_comPort, SP_MODE_READ_WRITE);
   if (result != SP_OK)
      throw std::runtime_error(StringExtensions::Build("A exception occurred while trying to open the Com-port '{0}'. Will not send data to the controller.", m_comPortName));

   sp_set_baudrate(m_comPort, m_comPortBaudRate);
   sp_set_bits(m_comPort, m_comPortDataBits);

   enum sp_parity parity;
   switch (m_comPortParity)
   {
   case Parity::None: parity = SP_PARITY_NONE; break;
   case Parity::Odd: parity = SP_PARITY_ODD; break;
   case Parity::Even: parity = SP_PARITY_EVEN; break;
   case Parity::Mark: parity = SP_PARITY_MARK; break;
   case Parity::Space: parity = SP_PARITY_SPACE; break;
   default: parity = SP_PARITY_NONE; break;
   }
   sp_set_parity(m_comPort, parity);

   int stopBits;
   switch (m_comPortStopBits)
   {
   case StopBits::One: stopBits = 1; break;
   case StopBits::Two: stopBits = 2; break;
   case StopBits::OnePointFive: stopBits = 1; break;
   default: stopBits = 1; break;
   }
   sp_set_stopbits(m_comPort, stopBits);

   if (m_comPortDtrEnable)
      sp_set_dtr(m_comPort, SP_DTR_ON);
   else
      sp_set_dtr(m_comPort, SP_DTR_OFF);

   std::this_thread::sleep_for(std::chrono::milliseconds(m_comPortOpenWaitMs));
   sp_flush(m_comPort, SP_BUF_BOTH);
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   bool commandModeOK = false;
   for (int attemptNr = 0; attemptNr < 20; attemptNr++)
   {
      uint8_t zeroCommand = 0;
      sp_blocking_write(m_comPort, &zeroCommand, 1, m_comPortTimeOutMs);
      std::this_thread::sleep_for(std::chrono::milliseconds(m_comPortHandshakeStartWaitMs));

      int bytesToRead = sp_input_waiting(m_comPort);
      if (bytesToRead > 0)
      {
         uint8_t response;
         int bytesRead = sp_blocking_read(m_comPort, &response, 1, m_comPortTimeOutMs);
         if (bytesRead == 1)
         {
            if (response == (uint8_t)'A' || response == (uint8_t)'N')
            {
               commandModeOK = true;
               break;
            }
         }

         std::vector<uint8_t> zeroBytes(3000, 0);
         sp_blocking_write(m_comPort, zeroBytes.data(), zeroBytes.size(), m_comPortTimeOutMs);
         std::this_thread::sleep_for(std::chrono::milliseconds(m_comPortHandshakeEndWaitMs));
         sp_flush(m_comPort, SP_BUF_INPUT);
      }
   }

   if (!commandModeOK)
   {
      Log::Exception(StringExtensions::Build("Could not put the controller on com-port '{0}' into the commandmode. Will not send data to the controller.", m_comPortName));
      DisconnectFromController();
      return;
   }

   SetupController();

   std::vector<uint8_t> receiveData(1);
   int bytesRead = -1;
   std::vector<uint8_t> commandData;

   commandData = { (uint8_t)'C' };
   result = sp_blocking_write(m_comPort, commandData.data(), 1, m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write clear command: {0}", sp_last_error_message()));

   receiveData.resize(1);
   bytesRead = -1;

   try
   {
      bytesRead = ReadPortWait(receiveData.data(), 0, 1);
   }
   catch (const std::exception& e)
   {
      throw std::runtime_error(StringExtensions::Build(
         "Expected 1 bytes after clearing the buffer of the {0}, but the read operation resulted in a exception. Will not send data to the controller.", GetXmlElementName()));
   }

   if (bytesRead != 1 || receiveData[0] != (uint8_t)'A')
   {
      throw std::runtime_error(StringExtensions::Build(
         "Expected a Ack (A) after clearing the buffer of the {0}, but received no answer or a unexpected answer. Will not send data to the controller.", GetXmlElementName()));
   }

   commandData = { (uint8_t)'O' };
   result = sp_blocking_write(m_comPort, commandData.data(), 1, m_comPortTimeOutMs);
   if (result < 0)
      throw std::runtime_error(StringExtensions::Build("Failed to write output command: {0}", sp_last_error_message()));

   receiveData.resize(1);
   bytesRead = -1;

   try
   {
      bytesRead = ReadPortWait(receiveData.data(), 0, 1);
   }
   catch (const std::exception& e)
   {
      throw std::runtime_error(StringExtensions::Build(
         "Expected 1 bytes after outputting the buffer of the {0} to the ledstrips, but the read operation resulted in a exception. Will not send data to the controller.",
         GetXmlElementName()));
   }

   if (bytesRead != 1 || receiveData[0] != (uint8_t)'A')
   {
      throw std::runtime_error(StringExtensions::Build(
         "Expected a Ack (A) after outputting the buffer of the {0} to the ledstrips, but received no answer or a unexpected answer. Will not send data to the controller.",
         GetXmlElementName()));
   }
}

void TeensyStripController::DisconnectFromController()
{
   if (m_comPort)
   {
      try
      {
         sp_close(m_comPort);
      }
      catch (...)
      {
      }
      sp_free_port(m_comPort);
      m_comPort = nullptr;
   }
}

int TeensyStripController::ReadPortWait(uint8_t* buffer, int bufferOffset, int numberOfBytes)
{
   for (int byteNumber = 0; byteNumber < numberOfBytes; byteNumber++)
   {
      int bytesRead = -1;
      uint8_t readBuffer;

      try
      {
         bytesRead = sp_blocking_read(m_comPort, &readBuffer, 1, m_comPortTimeOutMs);
      }
      catch (const std::exception& e)
      {
         throw std::runtime_error(StringExtensions::Build(
            "A exception occurred while trying to read byte {0} of {1} from Com-Port {2}.", std::to_string(byteNumber + 1), std::to_string(numberOfBytes), m_comPortName));
      }

      if (bytesRead != 1)
      {
         throw std::runtime_error(StringExtensions::Build("A exception occurred while trying to read byte {0} of {1} from Com-Port {2}. Tried to read 1 byte, but received {3} bytes.",
            std::to_string(byteNumber + 1), std::to_string(numberOfBytes), m_comPortName, std::to_string(bytesRead)));
      }

      buffer[bufferOffset + byteNumber] = readBuffer;
   }

   return numberOfBytes;
}

bool TeensyStripController::FromXml(const tinyxml2::XMLElement* element)
{
   if (!OutputControllerCompleteBase::FromXml(element))
      return false;

   auto loadElement = [&](const char* name, auto setter)
   {
      const tinyxml2::XMLElement* elem = element->FirstChildElement(name);
      if (elem && elem->GetText())
      {
         try
         {
            setter(elem->GetText());
         }
         catch (...)
         {
            return false;
         }
      }
      return true;
   };

   loadElement("ComPortName", [this](const char* text) { SetComPortName(text); });
   loadElement("ComPortBaudRate", [this](const char* text) { SetComPortBaudRate(std::stoi(text)); });
   loadElement("ComPortDataBits", [this](const char* text) { SetComPortDataBits(std::stoi(text)); });
   loadElement("ComPortTimeOutMs", [this](const char* text) { SetComPortTimeOutMs(std::stoi(text)); });
   loadElement("ComPortOpenWaitMs", [this](const char* text) { SetComPortOpenWaitMs(std::stoi(text)); });
   loadElement("ComPortHandshakeStartWaitMs", [this](const char* text) { SetComPortHandshakeStartWaitMs(std::stoi(text)); });
   loadElement("ComPortHandshakeEndWaitMs", [this](const char* text) { SetComPortHandshakeEndWaitMs(std::stoi(text)); });
   loadElement("ComPortDtrEnable", [this](const char* text) { SetComPortDtrEnable(std::string(text) == "true"); });

   loadElement("ComPortParity",
      [this](const char* text)
      {
         std::string parity = StringExtensions::ToLower(text);
         if (parity == "none")
            SetComPortParity(Parity::None);
         else if (parity == "odd")
            SetComPortParity(Parity::Odd);
         else if (parity == "even")
            SetComPortParity(Parity::Even);
         else if (parity == "mark")
            SetComPortParity(Parity::Mark);
         else if (parity == "space")
            SetComPortParity(Parity::Space);
      });

   loadElement("ComPortStopBits",
      [this](const char* text)
      {
         std::string stopBits = StringExtensions::ToLower(text);
         if (stopBits == "0" || stopBits == "none")
            SetComPortStopBits(StopBits::None);
         else if (stopBits == "1" || stopBits == "one")
            SetComPortStopBits(StopBits::One);
         else if (stopBits == "2" || stopBits == "two")
            SetComPortStopBits(StopBits::Two);
         else if (stopBits == "3" || stopBits == "onepointfive")
            SetComPortStopBits(StopBits::OnePointFive);
      });

   loadElement("NumberOfLedsStrip1", [this](const char* text) { SetNumberOfLedsStrip1(std::stoi(text)); });
   loadElement("NumberOfLedsStrip2", [this](const char* text) { SetNumberOfLedsStrip2(std::stoi(text)); });
   loadElement("NumberOfLedsStrip3", [this](const char* text) { SetNumberOfLedsStrip3(std::stoi(text)); });
   loadElement("NumberOfLedsStrip4", [this](const char* text) { SetNumberOfLedsStrip4(std::stoi(text)); });
   loadElement("NumberOfLedsStrip5", [this](const char* text) { SetNumberOfLedsStrip5(std::stoi(text)); });
   loadElement("NumberOfLedsStrip6", [this](const char* text) { SetNumberOfLedsStrip6(std::stoi(text)); });
   loadElement("NumberOfLedsStrip7", [this](const char* text) { SetNumberOfLedsStrip7(std::stoi(text)); });
   loadElement("NumberOfLedsStrip8", [this](const char* text) { SetNumberOfLedsStrip8(std::stoi(text)); });
   loadElement("NumberOfLedsStrip9", [this](const char* text) { SetNumberOfLedsStrip9(std::stoi(text)); });
   loadElement("NumberOfLedsStrip10", [this](const char* text) { SetNumberOfLedsStrip10(std::stoi(text)); });

   return true;
}

tinyxml2::XMLElement* TeensyStripController::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerCompleteBase::ToXml(doc);

   tinyxml2::XMLElement* numberOfLedsStrip1Element = doc.NewElement("NumberOfLedsStrip1");
   numberOfLedsStrip1Element->SetText(GetNumberOfLedsStrip1());
   element->InsertEndChild(numberOfLedsStrip1Element);

   tinyxml2::XMLElement* numberOfLedsStrip2Element = doc.NewElement("NumberOfLedsStrip2");
   numberOfLedsStrip2Element->SetText(GetNumberOfLedsStrip2());
   element->InsertEndChild(numberOfLedsStrip2Element);

   tinyxml2::XMLElement* numberOfLedsStrip3Element = doc.NewElement("NumberOfLedsStrip3");
   numberOfLedsStrip3Element->SetText(GetNumberOfLedsStrip3());
   element->InsertEndChild(numberOfLedsStrip3Element);

   tinyxml2::XMLElement* numberOfLedsStrip4Element = doc.NewElement("NumberOfLedsStrip4");
   numberOfLedsStrip4Element->SetText(GetNumberOfLedsStrip4());
   element->InsertEndChild(numberOfLedsStrip4Element);

   tinyxml2::XMLElement* numberOfLedsStrip5Element = doc.NewElement("NumberOfLedsStrip5");
   numberOfLedsStrip5Element->SetText(GetNumberOfLedsStrip5());
   element->InsertEndChild(numberOfLedsStrip5Element);

   tinyxml2::XMLElement* numberOfLedsStrip6Element = doc.NewElement("NumberOfLedsStrip6");
   numberOfLedsStrip6Element->SetText(GetNumberOfLedsStrip6());
   element->InsertEndChild(numberOfLedsStrip6Element);

   tinyxml2::XMLElement* numberOfLedsStrip7Element = doc.NewElement("NumberOfLedsStrip7");
   numberOfLedsStrip7Element->SetText(GetNumberOfLedsStrip7());
   element->InsertEndChild(numberOfLedsStrip7Element);

   tinyxml2::XMLElement* numberOfLedsStrip8Element = doc.NewElement("NumberOfLedsStrip8");
   numberOfLedsStrip8Element->SetText(GetNumberOfLedsStrip8());
   element->InsertEndChild(numberOfLedsStrip8Element);

   tinyxml2::XMLElement* numberOfLedsStrip9Element = doc.NewElement("NumberOfLedsStrip9");
   numberOfLedsStrip9Element->SetText(GetNumberOfLedsStrip9());
   element->InsertEndChild(numberOfLedsStrip9Element);

   tinyxml2::XMLElement* numberOfLedsStrip10Element = doc.NewElement("NumberOfLedsStrip10");
   numberOfLedsStrip10Element->SetText(GetNumberOfLedsStrip10());
   element->InsertEndChild(numberOfLedsStrip10Element);

   tinyxml2::XMLElement* comPortNameElement = doc.NewElement("ComPortName");
   comPortNameElement->SetText(GetComPortName().c_str());
   element->InsertEndChild(comPortNameElement);

   tinyxml2::XMLElement* comPortBaudRateElement = doc.NewElement("ComPortBaudRate");
   comPortBaudRateElement->SetText(GetComPortBaudRate());
   element->InsertEndChild(comPortBaudRateElement);

   tinyxml2::XMLElement* comPortParityElement = doc.NewElement("ComPortParity");
   switch (GetComPortParity())
   {
   case Parity::None: comPortParityElement->SetText("None"); break;
   case Parity::Odd: comPortParityElement->SetText("Odd"); break;
   case Parity::Even: comPortParityElement->SetText("Even"); break;
   case Parity::Mark: comPortParityElement->SetText("Mark"); break;
   case Parity::Space: comPortParityElement->SetText("Space"); break;
   }
   element->InsertEndChild(comPortParityElement);

   tinyxml2::XMLElement* comPortDataBitsElement = doc.NewElement("ComPortDataBits");
   comPortDataBitsElement->SetText(GetComPortDataBits());
   element->InsertEndChild(comPortDataBitsElement);

   tinyxml2::XMLElement* comPortStopBitsElement = doc.NewElement("ComPortStopBits");
   switch (GetComPortStopBits())
   {
   case StopBits::None: comPortStopBitsElement->SetText("None"); break;
   case StopBits::One: comPortStopBitsElement->SetText("One"); break;
   case StopBits::Two: comPortStopBitsElement->SetText("Two"); break;
   case StopBits::OnePointFive: comPortStopBitsElement->SetText("OnePointFive"); break;
   }
   element->InsertEndChild(comPortStopBitsElement);

   tinyxml2::XMLElement* comPortTimeOutMsElement = doc.NewElement("ComPortTimeOutMs");
   comPortTimeOutMsElement->SetText(GetComPortTimeOutMs());
   element->InsertEndChild(comPortTimeOutMsElement);

   tinyxml2::XMLElement* comPortOpenWaitMsElement = doc.NewElement("ComPortOpenWaitMs");
   comPortOpenWaitMsElement->SetText(GetComPortOpenWaitMs());
   element->InsertEndChild(comPortOpenWaitMsElement);

   tinyxml2::XMLElement* comPortHandshakeStartWaitMsElement = doc.NewElement("ComPortHandshakeStartWaitMs");
   comPortHandshakeStartWaitMsElement->SetText(GetComPortHandshakeStartWaitMs());
   element->InsertEndChild(comPortHandshakeStartWaitMsElement);

   tinyxml2::XMLElement* comPortHandshakeEndWaitMsElement = doc.NewElement("ComPortHandshakeEndWaitMs");
   comPortHandshakeEndWaitMsElement->SetText(GetComPortHandshakeEndWaitMs());
   element->InsertEndChild(comPortHandshakeEndWaitMsElement);

   tinyxml2::XMLElement* comPortDtrEnableElement = doc.NewElement("ComPortDtrEnable");
   comPortDtrEnableElement->SetText(GetComPortDtrEnable() ? "true" : "false");
   element->InsertEndChild(comPortDtrEnableElement);

   return element;
}

}