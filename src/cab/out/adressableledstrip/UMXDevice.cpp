#include "UMXDevice.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include <algorithm>
#include <cstring>

namespace DOF
{

const std::vector<std::pair<UMXDevice::LedChipset, int>> UMXDevice::LedsCaps = {
   { LedChipset::WS2811, 30000 },
   { LedChipset::WS2812, 30000 },
   { LedChipset::WS2812B, 30000 },
   { LedChipset::WS2813, 30000 },
   { LedChipset::SK6812, 30000 },
};

UMXDevice::UMXDevice()
   : m_longestDataLineNbLeds(0)
   , m_unitNo(0)
   , m_enabled(false)
   , m_maxDataLines(0)
   , m_maxNbLeds(INT_MAX)
   , m_ledChipset(LedChipset::WS2812B)
   , m_ledWizEquivalent(0)
   , m_numOutputs(0)
   , m_testOnReset(TestMode::None)
   , m_testOnResetDuration(0)
   , m_testOnConnect(TestMode::None)
   , m_testOnConnectDuration(0)
   , m_testBrightness(100)
   , m_compressionRatio(75)
   , m_totalLeds(0)
   , m_activityLed(false)
{
}

UMXDevice::~UMXDevice() { }

std::string UMXDevice::ToString() const
{
   return StringExtensions::Build("{0} (unit {1}, ledWiz {2}, {3} outputs)", { m_name, std::to_string(m_unitNo), std::to_string(m_ledWizEquivalent), std::to_string(m_numOutputs) });
}

bool UMXDevice::VerifySettings() { return true; }

void UMXDevice::ComputeNumOutputs()
{
   if (m_numOutputs > 0)
      return;
   m_numOutputs = 0;
   for (const auto& ledstrip : m_ledStrips)
      m_numOutputs += ledstrip.m_width * ledstrip.m_height * 3;
}

void UMXDevice::CreateDataLines()
{
   m_longestDataLineNbLeds = 0;

   m_dataLines.resize(m_maxDataLines);
   for (int num = 0; num < m_maxDataLines; num++)
      m_dataLines[num] = DataLine();

   for (const auto& ledstrip : m_ledStrips)
   {
      for (const auto& split : ledstrip.m_splits)
      {
         m_dataLines[split.m_dataLine].m_nbLeds += split.m_nbLeds;
         m_longestDataLineNbLeds = std::max(m_longestDataLineNbLeds, m_dataLines[split.m_dataLine].m_nbLeds);
      }
   }

   for (auto& line : m_dataLines)
   {
      line.m_nbValues = line.m_nbLeds * 3;
      line.m_oldValues.assign(line.m_nbValues, 255);
   }
}

void UMXDevice::ResetDataLines()
{
   for (auto& line : m_dataLines)
      line.m_oldValues.assign(line.m_nbValues, 255);
}

void UMXDevice::UpdateOutputs(const std::vector<uint8_t>& outputValues, bool full)
{
   if (!m_enabled)
      return;

   for (auto& line : m_dataLines)
   {
      if (line.m_nbValues > 0)
         line.m_values.clear();
   }

   int curOutputIndex = 0;
   for (const auto& ledstrip : m_ledStrips)
   {
      for (const auto& split : ledstrip.m_splits)
      {
         auto& line = m_dataLines[split.m_dataLine];
         int nbValues = split.m_nbLeds * 3;
         int endIndex = std::min(curOutputIndex + nbValues, static_cast<int>(outputValues.size()));
         line.m_values.insert(line.m_values.end(), outputValues.begin() + curOutputIndex, outputValues.begin() + endIndex);
         curOutputIndex += nbValues;
      }
   }

   std::vector<uint8_t> sendBuffer;
   uint8_t nbChangedLines = 0;

   for (int numLine = 0; numLine < static_cast<int>(m_dataLines.size()); numLine++)
   {
      auto& line = m_dataLines[numLine];
      if (line.m_nbLeds > 0)
      {
         if (full || line.m_values != line.m_oldValues)
         {
            nbChangedLines++;
            sendBuffer.push_back(static_cast<uint8_t>(numLine));

            std::vector<uint8_t> compressedLine = CompressDataLineValues(line.m_values);
            float compRatio = static_cast<float>(compressedLine.size()) / line.m_values.size();

            if (compRatio < m_compressionRatio * 0.01f)
            {
               Log::Instrumentation("UMX", StringExtensions::Build("Send compressed Mx Data (ratio {0}%)", std::to_string(static_cast<int>(compRatio * 100))));
               sendBuffer.push_back(1);
               sendBuffer.push_back(static_cast<uint8_t>(compressedLine.size() & 0xFF));
               sendBuffer.push_back(static_cast<uint8_t>(compressedLine.size() >> 8));
               sendBuffer.push_back(static_cast<uint8_t>(line.m_nbLeds & 0xFF));
               sendBuffer.push_back(static_cast<uint8_t>(line.m_nbLeds >> 8));
               sendBuffer.insert(sendBuffer.end(), compressedLine.begin(), compressedLine.end());
            }
            else
            {
               sendBuffer.push_back(0);
               sendBuffer.push_back(static_cast<uint8_t>(line.m_nbLeds & 0xFF));
               sendBuffer.push_back(static_cast<uint8_t>(line.m_nbLeds >> 8));
               sendBuffer.insert(sendBuffer.end(), line.m_values.begin(), line.m_values.end());
            }
         }
      }
   }

   if (nbChangedLines > 0 && !sendBuffer.empty())
   {
      std::vector<uint8_t> buffer = { nbChangedLines };
      buffer.insert(buffer.end(), sendBuffer.begin(), sendBuffer.end());
      SendCommand(UMXCommand::UMX_SendStripsData, buffer);
      WaitAck(static_cast<uint8_t>(UMXCommand::UMX_SendStripsData));
   }
   for (auto& line : m_dataLines)
   {
      if (line.m_nbLeds > 0)
         line.m_oldValues = line.m_values;
   }
}

std::vector<uint8_t> UMXDevice::CompressDataLineValues(const std::vector<uint8_t>& values)
{
   std::vector<uint8_t> newList;
   int num = 0;

   try
   {
      while (num < static_cast<int>(values.size()))
      {
         if (static_cast<int>(values.size()) - num <= 3)
         {
            newList.push_back(1);
            newList.push_back(0);
            newList.push_back(values[num++]);
            newList.push_back(values[num++]);
            newList.push_back(values[num++]);
            break;
         }
         else
         {
            uint8_t r = values[num++];
            uint8_t g = values[num++];
            uint8_t b = values[num++];
            int color = (r << 16) | (g << 8) | b;
            int sameColCount = 1;

            while (num + 2 < static_cast<int>(values.size()) && ((values[num] << 16) | (values[num + 1] << 8) | values[num + 2]) == color)
            {
               sameColCount++;
               num += 3;
               if (num >= static_cast<int>(values.size()))
                  break;
            }

            newList.push_back(static_cast<uint8_t>(sameColCount & 0xFF));
            newList.push_back(static_cast<uint8_t>(sameColCount >> 8));
            newList.push_back(r);
            newList.push_back(g);
            newList.push_back(b);
         }
      }
   }
   catch (...)
   {
      newList.clear();
   }

   return newList;
}

}
