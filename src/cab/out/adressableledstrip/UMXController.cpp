#include "UMXController.h"
#include "UMXControllerAutoConfigurator.h"
#include "../../Cabinet.h"
#include "../OutputControllerList.h"
#include "../../toys/ToyList.h"
#include "../../toys/hardware/LedStrip.h"
#include "../../toys/lwequivalent/LedWizEquivalent.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <algorithm>
#include <thread>

namespace DOF
{

const int UMXController::MinDataLineDelayMs;

UMXController::UMXController()
   : m_number(-1)
   , m_dev(nullptr)
   , m_lastTimeInit(false)
   , m_longestDataLineDelayMs(16)
{
}

UMXController::~UMXController() { }

void UMXController::SetNumber(int value)
{
   std::lock_guard<std::mutex> lock(m_numberUpdateLocker);

   if (m_number != value)
   {
      if (GetName().empty() || GetName() == StringExtensions::Build("UMX Controller {0:00}", std::to_string(m_number)))
         SetName(StringExtensions::Build("UMX Controller {0:00}", std::to_string(value)));

      m_number = value;

      auto devices = UMXControllerAutoConfigurator::AllDevices();
      auto it = std::find_if(devices.begin(), devices.end(), [this](UMXDevice* d) { return d->UnitNo() == m_number; });
      if (it != devices.end())
         m_dev = *it;
   }
}

int UMXController::GetNumberOfConfiguredOutputs() { return m_dev ? m_dev->NumOutputs() : 0; }

void UMXController::ConnectToController()
{
   try
   {
      DisconnectFromController();
   }
   catch (const std::exception& e)
   {
      Log::Warning(StringExtensions::Build("UMX controller {0} could not turn its outputs off before connecting: {1}", GetName(), e.what()));
   }
   if (m_dev && m_dev->IsLost())
      throw std::runtime_error(StringExtensions::Build("UMX controller {0} lost its device, which cannot be opened again", GetName()));
   if (m_dev)
      m_dev->ResetDataLines();
}

void UMXController::DisconnectFromController()
{
   if (m_inUseState == InUseState::Running && m_dev)
   {
      m_dev->SendCommand(UMXDevice::UMXCommand::UMX_AllOff);
      m_disconnectTurnedOff = true;
   }
}

bool UMXController::VerifySettings()
{
   if (m_dev == nullptr)
      return false;
   if (m_dev->NumOutputs() == 0)
      return false;
   if (!m_dev->VerifySettings())
      return false;
   return true;
}

void UMXController::UpdateOutputs(const std::vector<uint8_t>& outputValues)
{
   auto now = std::chrono::steady_clock::now();
   int delay = m_lastTimeInit ? static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTime).count()) : 0;

   if (m_lastTimeInit && delay < m_longestDataLineDelayMs)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(m_longestDataLineDelayMs - delay));
      if (!m_forceFullTransfer)
      {
         m_transferSkipped = true;
         return;
      }
   }

   if (!m_dev)
      throw std::runtime_error(StringExtensions::Build("UMX controller {0} has no device", GetName()));
   m_dev->UpdateOutputs(outputValues, m_forceFullTransfer);
   m_lastTime = std::chrono::steady_clock::now();
   m_lastTimeInit = true;
}

void UMXController::UpdateCabinetFromConfig(Cabinet* cabinet)
{
   if (!m_dev)
      return;

   m_dev->Initialize();

   if (!m_dev->GetEnabled())
   {
      Log::Write(StringExtensions::Build("UMX Device {0} is disabled, UMXController {1} #{2} won't be added to cabinet", { m_dev->m_name, GetName(), std::to_string(m_number) }));
      return;
   }

   if (cabinet->GetOutputControllers()->Contains(GetName()))
   {
      Log::Write(StringExtensions::Build("UMX Controller {0} already exists in the cabinet, won't add a new one", GetName()));
      return;
   }

   if (m_dev->m_testOnConnect != UMXDevice::TestMode::None)
   {
      std::vector<uint8_t> parameters = {
         static_cast<uint8_t>(m_dev->m_testOnConnect),
         m_dev->m_testOnConnectDuration,
      };
      try
      {
         m_dev->SendCommand(UMXDevice::UMXCommand::UMX_StartTest, parameters);
      }
      catch (const std::exception& e)
      {
         if (m_dev->IsLost())
            throw;
         Log::Warning(StringExtensions::Build("UMX controller {0} could not start its connect test: {1}", GetName(), e.what()));
      }
   }

   Log::Write(StringExtensions::Build("Detected and added UMX Controller Nr. {0} with name {1}", std::to_string(m_number), GetName()));
   cabinet->GetOutputControllers()->push_back(this);

   Log::Write(StringExtensions::Build("UMX Infos : Name {0}, UMX Version {1}.{2}.{3}, MaxDataLines {4}, MaxNbLeds {5}",
      { m_dev->m_name, std::to_string(m_dev->m_umxVersion.major), std::to_string(m_dev->m_umxVersion.minor), std::to_string(m_dev->m_umxVersion.build), std::to_string(m_dev->m_maxDataLines),
         std::to_string(m_dev->m_maxNbLeds) }));
   Log::Write(StringExtensions::Build("UMX Config : Enabled {0}, LedChipset {1}, LedWizEquivalent {2}",
      { std::to_string(m_dev->m_enabled), std::to_string(static_cast<int>(m_dev->m_ledChipset)), std::to_string(m_dev->m_ledWizEquivalent) }));
   Log::Write(StringExtensions::Build("TestOnReset {0}({1}s), TestOnConnect {2}({3}s), TestBrightness: {4}",
      { std::to_string(static_cast<int>(m_dev->m_testOnReset)), std::to_string(m_dev->m_testOnResetDuration), std::to_string(static_cast<int>(m_dev->m_testOnConnect)),
         std::to_string(m_dev->m_testOnConnectDuration), std::to_string(m_dev->m_testBrightness) }));
   Log::Write(StringExtensions::Build("CompressionRatio {0}%", std::to_string(m_dev->m_compressionRatio)));
   Log::Write(StringExtensions::Build("{0} ledstrips :", std::to_string(m_dev->m_ledStrips.size())));

   for (int numstrip = 0; numstrip < static_cast<int>(m_dev->m_ledStrips.size()); numstrip++)
   {
      auto& ledstrip = m_dev->m_ledStrips[numstrip];
      Log::Write(StringExtensions::Build("\t[{0}] {1} => W/H:{2}/{3} ({4} leds), FirstLed: {5}, Dof:{6}, Brightness:{7}",
         { std::to_string(numstrip), ledstrip.m_name, std::to_string(ledstrip.m_width), std::to_string(ledstrip.m_height), std::to_string(ledstrip.NbLeds()),
            std::to_string(ledstrip.m_firstLedIndex), std::to_string(ledstrip.m_dofOutputNum), std::to_string(ledstrip.m_brightness) }));

      if (ledstrip.m_splits.size() == 1)
      {
         Log::Write(StringExtensions::Build("\t\t1 split : {0} leds on line {1}", std::to_string(ledstrip.m_splits.back().m_nbLeds), std::to_string(ledstrip.m_splits.back().m_dataLine)));
      }
      else
      {
         Log::Write(StringExtensions::Build("\t\t{0} splits :", std::to_string(ledstrip.m_splits.size())));
         for (const auto& split : ledstrip.m_splits)
            Log::Write(StringExtensions::Build("\t\t\t{0} leds on line {1}", std::to_string(split.m_nbLeds), std::to_string(split.m_dataLine)));
      }
   }
   Log::Write(StringExtensions::Build("{0} leds, {1} outputs configured", std::to_string(m_dev->m_totalLeds), std::to_string(m_dev->NumOutputs())));

   m_dev->CreateDataLines();

   Log::Instrumentation("UMX", StringExtensions::Build("{0} Output lines generated", std::to_string(m_dev->m_dataLines.size())));
   for (const auto& line : m_dev->m_dataLines)
      Log::Instrumentation("UMX", StringExtensions::Build("\t{0} leds", std::to_string(line.m_nbLeds)));

   int ledsRefresh = 30000;
   for (const auto& cap : UMXDevice::LedsCaps)
   {
      if (cap.first == m_dev->m_ledChipset)
      {
         ledsRefresh = cap.second;
         break;
      }
   }

   if (m_dev->m_longestDataLineNbLeds > 0)
      m_longestDataLineDelayMs = 1000 / (ledsRefresh / m_dev->m_longestDataLineNbLeds);
   else
      m_longestDataLineDelayMs = 16;
   m_longestDataLineDelayMs = std::max(m_longestDataLineDelayMs, MinDataLineDelayMs);

   Log::Instrumentation("UMX",
      StringExtensions::Build(
         "\tLongest Dataline is {0} leds (controller delay is set at {1} ms)", std::to_string(m_dev->m_longestDataLineNbLeds), std::to_string(m_longestDataLineDelayMs)));

   bool foundLwe = false;
   for (auto* toy : *cabinet->GetToys())
   {
      LedWizEquivalent* lwe = dynamic_cast<LedWizEquivalent*>(toy);
      if (lwe && lwe->GetLedWizNumber() == m_dev->m_ledWizEquivalent)
      {
         foundLwe = true;
         break;
      }
   }

   if (!foundLwe)
   {
      LedWizEquivalent* lwe = new LedWizEquivalent();
      lwe->SetLedWizNumber(m_dev->m_ledWizEquivalent);
      lwe->SetName(StringExtensions::Build("{0}#{1} Equivalent", m_dev->m_name, std::to_string(m_dev->m_unitNo)));

      if (!cabinet->GetToys()->Contains(lwe->GetName()))
      {
         cabinet->GetToys()->AddToy(lwe);
         Log::Write(StringExtensions::Build("Added LedwizEquivalent Nr. {0} with name {1} for UMXController Nr. {2}, {3}",
            { std::to_string(lwe->GetLedWizNumber()), lwe->GetName(), std::to_string(m_dev->m_unitNo), std::to_string(m_dev->m_ledStrips.size()) }));
      }

      for (const auto& ledstripDesc : m_dev->m_ledStrips)
      {
         LedStrip* ledstrip = new LedStrip();
         ledstrip->SetBrightness(ledstripDesc.m_brightness);
         ledstrip->SetColorOrder(ledstripDesc.m_colorOrder);
         ledstrip->SetFirstLedNumber(ledstripDesc.m_firstLedIndex + 1);
         ledstrip->SetHeight(ledstripDesc.m_height);
         ledstrip->SetWidth(ledstripDesc.m_width);
         ledstrip->SetLedStripArrangement(ledstripDesc.m_arrangement);
         ledstrip->SetFadingCurveName(Curve::CurveTypeEnumToString(ledstripDesc.m_fadeMode));
         ledstrip->SetName(StringExtensions::Build("Ledstrip_StartLed{0}", std::to_string(ledstripDesc.m_firstLedIndex + 1)));
         ledstrip->SetOutputControllerName(GetName());

         cabinet->GetToys()->AddToy(ledstrip);

         LedWizEquivalentOutput* lweo = new LedWizEquivalentOutput();
         lweo->SetOutputName(ledstrip->GetName());
         lweo->SetLedWizEquivalentOutputNumber(ledstripDesc.m_dofOutputNum);
         lwe->GetOutputs().AddOutput(lweo);
      }
   }

}

tinyxml2::XMLElement* UMXController::ToXml(tinyxml2::XMLDocument& doc) const
{
   tinyxml2::XMLElement* element = OutputControllerCompleteBase::ToXml(doc);

   tinyxml2::XMLElement* numberElement = doc.NewElement("Number");
   numberElement->SetText(m_number);
   element->InsertEndChild(numberElement);

   return element;
}

bool UMXController::FromXml(const tinyxml2::XMLElement* element)
{
   if (!OutputControllerCompleteBase::FromXml(element))
      return false;

   const tinyxml2::XMLElement* numberElement = element->FirstChildElement("Number");
   if (numberElement && numberElement->GetText())
   {
      try
      {
         int number = std::stoi(numberElement->GetText());
         SetNumber(number);
      }
      catch (...)
      {
         return false;
      }
   }

   return true;
}

}
