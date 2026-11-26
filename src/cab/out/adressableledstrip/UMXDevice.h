#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../../toys/layer/RGBOrderEnum.h"
#include "../../toys/layer/StripArrangementEnum.h"
#include "../../../general/Curve.h"

namespace DOF
{

class UMXDevice
{
public:
   enum class UMXCommand
   {
      UMX_Handshake,
      UMX_GetInfos,
      UMX_GetConfig,
      UMX_AllOff,
      UMX_SendStripsData,
      UMX_StartTest
   };

   enum class TestMode
   {
      None,
      RGB,
      Colors,
      RGBLaser
   };

   enum class LedChipset
   {
      WS2811,
      WS2812,
      WS2812B,
      WS2813,
      WS2815,
      SK6812
   };

   static const std::vector<std::pair<LedChipset, int>> LedsCaps;

   struct LedStripSplit
   {
      int m_dataLine = 0;
      int m_nbLeds = 0;
   };

   struct LedStripDescriptor
   {
      std::string m_name;
      int m_width = 0;
      int m_height = 0;
      int NbLeds() const { return m_width * m_height; }
      int m_firstLedIndex = 0;
      uint8_t m_dofOutputNum = 0;
      Curve::CurveTypeEnum m_fadeMode = Curve::CurveTypeEnum::SwissLizardsLedCurve;
      LedStripArrangementEnum m_arrangement = LedStripArrangementEnum::LeftRightTopDown;
      RGBOrderEnum m_colorOrder = RGBOrderEnum::RBG;
      uint8_t m_brightness = 100;
      std::vector<LedStripSplit> m_splits;

      int FirstDataline() const
      {
         if (m_splits.empty())
            return 0;
         int minLine = m_splits[0].m_dataLine;
         for (size_t i = 1; i < m_splits.size(); i++)
         {
            if (m_splits[i].m_dataLine < minLine)
               minLine = m_splits[i].m_dataLine;
         }
         return minLine;
      }
   };

   struct DataLine
   {
      int m_nbLeds = 0;
      int m_nbValues = 0;
      std::vector<uint8_t> m_oldValues;
      std::vector<uint8_t> m_values;
   };

   UMXDevice();
   virtual ~UMXDevice();

   std::string ToString() const;
   bool GetEnabled() const { return m_enabled; }
   int UnitNo() const { return m_unitNo; }
   int NumOutputs() const { return m_numOutputs; }
   bool VerifySettings();

   void ComputeNumOutputs();
   void CreateDataLines();
   void ResetDataLines();
   void UpdateOutputs(const std::vector<uint8_t>& outputValues, bool full = false);

   virtual void Initialize() = 0;
   virtual void SendCommand(UMXCommand command, const std::vector<uint8_t>& parameters = { }) = 0;
   virtual void WaitAck(uint8_t command) = 0;

   std::vector<LedStripDescriptor> m_ledStrips;
   std::vector<DataLine> m_dataLines;
   int m_longestDataLineNbLeds;

   std::string m_name;
   int16_t m_unitNo;
   bool m_enabled;
   struct Version
   {
      int major = 0;
      int minor = 0;
      int build = 0;
   };
   Version m_umxVersion;
   uint8_t m_maxDataLines;
   int m_maxNbLeds;
   LedChipset m_ledChipset;
   int m_ledWizEquivalent;
   int m_numOutputs;
   TestMode m_testOnReset;
   uint8_t m_testOnResetDuration;
   TestMode m_testOnConnect;
   uint8_t m_testOnConnectDuration;
   uint8_t m_testBrightness;
   uint8_t m_compressionRatio;
   int m_totalLeds;
   bool m_activityLed;

private:
   std::vector<uint8_t> CompressDataLineValues(const std::vector<uint8_t>& values);
};

}
