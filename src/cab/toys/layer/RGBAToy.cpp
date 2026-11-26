#include "RGBAToy.h"
#include "../../Cabinet.h"
#include "../../CabinetOutputList.h"
#include "../../out/IOutput.h"
#include "../../../Log.h"
#include "../../../general/StringExtensions.h"
#include "AlphaMappingTable.h"

#include <algorithm>

namespace DOF
{

RGBAToy::RGBAToy()
   : m_redOutput(nullptr)
   , m_greenOutput(nullptr)
   , m_blueOutput(nullptr)
{
}

RGBAToy::~RGBAToy() { }

void RGBAToy::Init(Cabinet* cabinet)
{
   if (cabinet == nullptr)
   {
      Log::Warning("RGBAToy::Init - Cabinet is null");
      return;
   }

   Log::Write(StringExtensions::Build("Initializing RGBAToy: {0}", GetName()));

   m_redOutput = cabinet->GetOutputs()->GetByName(m_outputNameRed);
   m_greenOutput = cabinet->GetOutputs()->GetByName(m_outputNameGreen);
   m_blueOutput = cabinet->GetOutputs()->GetByName(m_outputNameBlue);


   Reset();
}

RGBColor RGBAToy::GetResultingData()
{
   if (m_layers.GetLayerNumbers().size() > 0)
   {
      float red = 0;
      float green = 0;
      float blue = 0;
      std::vector<int> layerNumbers = m_layers.GetLayerNumbers();
      for (int layerNumber : layerNumbers)
      {
         RGBAColor* layer = m_layers.GetLayer(layerNumber, 1);
         if (layer == nullptr)
            continue;
         const RGBAColor& layerColor = layer[0];
         int alpha = layerColor.GetAlpha();
         if (alpha != 0)
         {
            int negAlpha = 255 - alpha;
            red = AlphaMappingTable::AlphaMapping[negAlpha][(int)red] + AlphaMappingTable::AlphaMapping[alpha][layerColor.GetRed()];
            green = AlphaMappingTable::AlphaMapping[negAlpha][(int)green] + AlphaMappingTable::AlphaMapping[alpha][layerColor.GetGreen()];
            blue = AlphaMappingTable::AlphaMapping[negAlpha][(int)blue] + AlphaMappingTable::AlphaMapping[alpha][layerColor.GetBlue()];
         }
      }

      return RGBColor((int)red, (int)green, (int)blue);
   }
   else
   {
      return RGBColor(0, 0, 0);
   }
}

void RGBAToy::Reset()
{
   for (int layerNumber : m_layers.GetLayerNumbers())
      m_layers.GetLayer(layerNumber, 1)[0] = RGBAColor();
   if (m_redOutput != nullptr)
      m_redOutput->SetOutput(0);
   if (m_greenOutput != nullptr)
      m_greenOutput->SetOutput(0);
   if (m_blueOutput != nullptr)
      m_blueOutput->SetOutput(0);
}

void RGBAToy::Finish()
{
   Log::Write(StringExtensions::Build("Finishing RGBAToy: {0}", GetName()));
   Reset();
}

void RGBAToy::UpdateToy() { UpdateOutputs(); }

void RGBAToy::UpdateOutputs()
{
   RGBColor rgb = GetResultingData();

   if (m_redOutput != nullptr)
      m_redOutput->SetOutput(static_cast<uint8_t>(rgb.GetRed()));

   if (m_greenOutput != nullptr)
      m_greenOutput->SetOutput(static_cast<uint8_t>(rgb.GetGreen()));

   if (m_blueOutput != nullptr)
      m_blueOutput->SetOutput(static_cast<uint8_t>(rgb.GetBlue()));
}

}