#include "FT245RBitbangController.h"
#include "../../Cabinet.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "../../../general/MathExtensions.h"
#include "../../overrides/TableOverrideSettings.h"
#include "../../schedules/ScheduledSettings.h"
#include "../Output.h"
#include "../OutputList.h"
#include <stdexcept>
#include <chrono>
#include <tinyxml2/tinyxml2.h>

namespace DOF
{

FT245RBitbangController::FT245RBitbangController()
   : m_id(0)
   , m_newValue(0)
   , m_currentValue(255)
   , m_keepUpdaterThreadAlive(false)
   , m_updaterThreadFinished(false)
   , m_firstTryFailCnt(0)
   , m_updaterThread(nullptr)
   , m_ftdi(nullptr)
{
   SetOutputs(new OutputList());
}

FT245RBitbangController::~FT245RBitbangController() { FinishUpdaterThread(); }

void FT245RBitbangController::SetId(int value)
{
   if (!MathExtensions::IsBetween(value, 0, 9))
   {
      throw std::runtime_error(StringExtensions::Build("FT245RBitbangController Ids must be between 0-9. The supplied Id {0} is out of range.", std::to_string(value)));
   }
   m_id = value;
}

void FT245RBitbangController::Init(Cabinet* cabinet)
{
   if (StringExtensions::IsNullOrWhiteSpace(m_serialNumber))
   {
      Log::Exception(StringExtensions::Build("Could not initialize FT245RBitbangController {0}. SerialNumber has not been set.", GetName()));
      return;
   }
   AddOutputs();
   m_finishError.clear();
   InitUpdaterThread();
   Log::Write(StringExtensions::Build("FT245RBitbangController {0} with serial number {1} has been initialized and the updater thread has been started.", GetName(), m_serialNumber));
}

void FT245RBitbangController::Finish()
{
   FinishUpdaterThread();
   Log::Write(StringExtensions::Build("FT245RBitbangController {0} with serial number {1} has been finished and the updater thread has been terminated.", GetName(), m_serialNumber));
   if (!m_finishError.empty())
   {
      std::string error = m_finishError;
      m_finishError.clear();
      throw std::runtime_error(error);
   }
}

void FT245RBitbangController::Update()
{
   if (m_newValue != m_currentValue)
   {
      UpdaterThreadSignal();
   }
}

void FT245RBitbangController::AddOutputs()
{
   OutputList* outputs = GetOutputs();
   if (!outputs)
      return;

   for (int i = 1; i <= 8; i++)
   {
      bool found = false;
      for (auto* output : *outputs)
      {
         if (output->GetNumber() == i)
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         Output* newOutput = new Output();
         newOutput->SetName(StringExtensions::Build("{0}.{1:00}", GetName(), std::to_string(i)));
         newOutput->SetNumber(i);
         outputs->Add(newOutput);
      }
   }
}

void FT245RBitbangController::OnOutputValueChanged(IOutput* output)
{
   IOutput* on = output;

   on = TableOverrideSettings::GetInstance()->GetNewRecalculatedOutput(on, 40, m_id);
   IOutput* overriddenOutput = on;

   on = ScheduledSettings::GetInstance().GetNewRecalculatedOutput(on, 40, m_id);

   int number = on->GetNumber();
   uint8_t value = on->GetOutput();
   if (on != overriddenOutput)
      delete on;
   if (overriddenOutput != output)
      delete overriddenOutput;

   if (!MathExtensions::IsBetween(number, 1, 8))
   {
      throw std::runtime_error(StringExtensions::Build(
         "FT245RBitbangController output numbers must be in the range of 1-8. The supplied output number {0} for FT245RBitbangController with serial number {1} is out of range.",
         std::to_string(number), m_serialNumber));
   }

   std::lock_guard<std::mutex> lock(m_valueChangeLocker);

   if (value > 0)
   {
      m_newValue = m_newValue | (1 << (number - 1));
   }
   else
   {
      m_newValue = m_newValue & ~(1 << (number - 1));
   }
}

bool FT245RBitbangController::GetUpdaterThreadIsActive() const { return m_updaterThread && m_updaterThread->joinable() && m_keepUpdaterThreadAlive; }

void FT245RBitbangController::InitUpdaterThread()
{
   if (!GetUpdaterThreadIsActive())
   {
      m_keepUpdaterThreadAlive = true;
      m_updaterThreadFinished = false;
      try
      {
         m_updaterThread = new std::thread(&FT245RBitbangController::UpdaterThreadDoIt, this);
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("FT245RBitbangController {0} named {1} updater thread could not start.", m_serialNumber, GetName()));
         throw std::runtime_error(StringExtensions::Build("FT245RBitbangController {0} named {1} updater thread could not start.", m_serialNumber, GetName()));
      }
   }
}

void FT245RBitbangController::FinishUpdaterThread()
{
   if (m_updaterThread)
   {
      try
      {
         m_keepUpdaterThreadAlive = false;
         UpdaterThreadSignal();

         if (m_updaterThread->joinable())
         {
            auto start = std::chrono::steady_clock::now();
            while (!m_updaterThreadFinished && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000)
            {
               std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            m_updaterThread->join();
         }
         delete m_updaterThread;
         m_updaterThread = nullptr;
      }
      catch (const std::exception& e)
      {
         Log::Exception(StringExtensions::Build("A error occurred during termination of FT245RBitbangController ({0}) updater thread.", m_serialNumber));
         throw std::runtime_error(StringExtensions::Build("A error occurred during termination of FT245RBitbangController ({0}) updater thread.", m_serialNumber));
      }
   }
}

void FT245RBitbangController::UpdaterThreadSignal()
{
   std::lock_guard<std::mutex> lock(m_updaterThreadLocker);
   m_updaterThreadCondition.notify_one();
}

void FT245RBitbangController::UpdaterThreadDoIt()
{
   Connect();
   if (!m_ftdi)
   {
      Log::Warning(StringExtensions::Build("No connection to FTDI chip {0}. Updater thread will terminate.", m_serialNumber));
      m_updaterThreadFinished = true;
      return;
   }

   bool started = true;
   try
   {
      SendUpdate(0);
   }
   catch (const std::exception&)
   {
      Log::Exception(StringExtensions::Build("Could not send initial update to FTDI chip {0}. Updater thread will terminate.", m_serialNumber));
      started = false;
   }

   while (started && m_keepUpdaterThreadAlive)
   {
      m_currentValue = m_newValue.load();

      try
      {
         SendUpdate(m_currentValue);
      }
      catch (const std::exception& e)
      {
         if (m_firstTryFailCnt < 5)
         {
            Log::Exception(StringExtensions::Build("Could not send update to FTDI chip {0} on first try. Will reconnect and send update again.", m_serialNumber));
            m_firstTryFailCnt++;
            if (m_firstTryFailCnt == 5)
            {
               Log::Warning(StringExtensions::Build("Will not log further warnings on first try failures when sending data to FTDI chip {0}.", m_serialNumber));
            }
         }

         try
         {
            Disconnect();
            Connect();
         }
         catch (const std::exception& ec)
         {
            Log::Exception(StringExtensions::Build("Could not send update to FTDI chip {0}. Tried to reconnect to device but failed. Updater thread will terminate.", m_serialNumber));
            break;
         }

         if (m_ftdi)
         {
            try
            {
               m_currentValue = m_newValue.load();
               SendUpdate(m_currentValue);
            }
            catch (const std::exception& ee)
            {
               Log::Exception(StringExtensions::Build(
                  "Could not send update to FTDI chip {0}. Reconnect to device worked, but sending the update did fail again. Updater thread will terminate.", m_serialNumber));
               break;
            }
         }
         else
         {
            Log::Exception(StringExtensions::Build("Could not send update to FTDI chip {0}. Tried to reconnect to device but failed. Updater thread will terminate.", m_serialNumber));
            break;
         }
      }

      if (m_keepUpdaterThreadAlive)
      {
         std::unique_lock<std::mutex> lock(m_updaterThreadLocker);
         m_updaterThreadCondition.wait_for(lock, std::chrono::milliseconds(50), [this]() { return m_newValue != m_currentValue || !m_keepUpdaterThreadAlive; });
      }
   }

   std::string finishError;
   try
   {
      SendUpdate(0);
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Final update to turn off all output for FTDI chip {0} failed.", m_serialNumber));
      finishError = StringExtensions::Build("FT245RBitbangController {0} could not turn its outputs off: {1}", GetName(), e.what());
   }
   if (!Disconnect() && !finishError.empty())
      m_finishError = finishError;
   m_updaterThreadFinished = true;
}

void FT245RBitbangController::Connect()
{
   std::lock_guard<std::recursive_mutex> lock(m_ftdiLocker);

   if (m_ftdi)
   {
      Disconnect();
   }

   if (StringExtensions::IsNullOrWhiteSpace(m_serialNumber))
   {
      Log::Exception(StringExtensions::Build("The SerialNumber has not been set for the FT245RBitbangController named {0}. Cant connect to device.", GetName()));
      return;
   }

   m_ftdi = new FTDI();

   try
   {
      FTDI::FT_STATUS status = m_ftdi->OpenEx(m_serialNumber, FTDI::FT_OPEN_BY_SERIAL_NUMBER);
      if (status != FTDI::FT_OK)
      {
         throw std::runtime_error("Failed to open FTDI device");
      }
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Could not open the connection to FTDI chip with serial number {0}.", m_serialNumber));
      delete m_ftdi;
      m_ftdi = nullptr;
      return;
   }

   try
   {
      // C# uses FT_BIT_MODE_SYNC_BITBANG (0x04) via proprietary FTD2XX.dll. libftdi requires async bitbang (0x01) for equivalent behavior.
      FTDI::FT_STATUS status = m_ftdi->SetBitMode(0xFF, 0x01);
      if (status != FTDI::FT_OK)
      {
         throw std::runtime_error("Failed to set bitbang mode");
      }
   }
   catch (const std::exception& e)
   {
      Log::Exception(StringExtensions::Build("Could not set the bitmode to bitbang for FTDI chip with serial number {0}.", m_serialNumber));
      Disconnect();
      delete m_ftdi;
      m_ftdi = nullptr;
      return;
   }

   Log::Write(StringExtensions::Build("Connection to FTDI chip {0} established.", m_serialNumber));
}

void FT245RBitbangController::SendUpdate(uint8_t outputData)
{
   std::lock_guard<std::recursive_mutex> lock(m_ftdiLocker);

   if (m_ftdi)
   {
      uint32_t numBytes = 0;
      uint8_t out[1] = { outputData };

      FTDI::FT_STATUS status = m_ftdi->Write(out, 1, numBytes);
      if (status != FTDI::FT_OK)
      {
         throw std::runtime_error("FTDI write failed");
      }
      if (numBytes != 1)
      {
         throw std::runtime_error(StringExtensions::Build(
            "Wrong number of written bytes (expected 1, received {0}) was returned when sending data to the FTDI chip {1}", std::to_string(numBytes), m_serialNumber));
      }
   }
   else
      throw std::runtime_error(StringExtensions::Build("FTDI chip {0} is not connected", m_serialNumber));
}

bool FT245RBitbangController::Disconnect()
{
   std::lock_guard<std::recursive_mutex> lock(m_ftdiLocker);

   bool turnedOff = false;
   if (m_ftdi)
   {
      try
      {
         SendUpdate(0);
         turnedOff = true;
      }
      catch (...)
      {
      }

      if (m_ftdi->IsOpen())
      {
         try
         {
            m_ftdi->Close();
         }
         catch (const std::exception&)
         {
            Log::Exception(StringExtensions::Build("A exception occurred when closing the FTDI chip {0}.", m_serialNumber));
         }
      }
      delete m_ftdi;
      m_ftdi = nullptr;
      Log::Write(StringExtensions::Build("Connection to FTDI chip {0} closed.", m_serialNumber));
   }
   return turnedOff;
}

tinyxml2::XMLElement* FT245RBitbangController::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = doc.NewElement(GetXmlElementName().c_str());

   if (!GetName().empty())
   {
      tinyxml2::XMLElement* nameElement = doc.NewElement("Name");
      nameElement->SetText(GetName().c_str());
      element->InsertEndChild(nameElement);
   }

   if (!m_serialNumber.empty())
   {
      tinyxml2::XMLElement* serialElement = doc.NewElement("SerialNumber");
      serialElement->SetText(m_serialNumber.c_str());
      element->InsertEndChild(serialElement);
   }

   if (!m_description.empty())
   {
      tinyxml2::XMLElement* descElement = doc.NewElement("Description");
      descElement->SetText(m_description.c_str());
      element->InsertEndChild(descElement);
   }

   if (m_id != 0)
   {
      tinyxml2::XMLElement* idElement = doc.NewElement("Id");
      idElement->SetText(m_id);
      element->InsertEndChild(idElement);
   }

   return element;
}

bool FT245RBitbangController::FromXml(const tinyxml2::XMLElement* element)
{
   if (!element)
      return false;

   const char* name = element->Attribute("Name");
   if (name)
   {
      SetName(name);
   }

   const tinyxml2::XMLElement* serialElement = element->FirstChildElement("SerialNumber");
   if (serialElement && serialElement->GetText())
   {
      m_serialNumber = serialElement->GetText();
   }

   const tinyxml2::XMLElement* descElement = element->FirstChildElement("Description");
   if (descElement && descElement->GetText())
   {
      m_description = descElement->GetText();
   }

   const tinyxml2::XMLElement* idElement = element->FirstChildElement("Id");
   if (idElement && idElement->GetText())
   {
      try
      {
         int id = std::stoi(idElement->GetText());
         SetId(id);
      }
      catch (...)
      {
         return false;
      }
   }

   return true;
}

}
