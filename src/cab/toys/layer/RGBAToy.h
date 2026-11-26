#pragma once

#include "../ToyBaseUpdatable.h"
#include "../IRGBAToy.h"
#include "../IRGBOutputToy.h"
#include "ILayerToy.h"
#include "../../../general/color/RGBAColor.h"
#include "../../../general/color/RGBColor.h"
#include <vector>

namespace DOF
{

class IOutput;

class RGBAToy : public ToyBaseUpdatable, public IRGBOutputToy, public IRGBAToy
{
public:
   RGBAToy();
   virtual ~RGBAToy();

   virtual void Init(Cabinet* cabinet) override;
   virtual void Reset() override;
   virtual void Finish() override;
   virtual void UpdateToy() override;
   virtual int GetOutputCount() const override { return 3; }
   virtual void UpdateOutputs() override;
   virtual LayerDictionary<RGBAColor>& GetLayers() override { return m_layers; }
   virtual const LayerDictionary<RGBAColor>& GetLayers() const override { return m_layers; }
   void SetRedOutput(IOutput* output) { m_redOutput = output; }
   void SetGreenOutput(IOutput* output) { m_greenOutput = output; }
   void SetBlueOutput(IOutput* output) { m_blueOutput = output; }

   IOutput* GetRedOutput() const { return m_redOutput; }
   IOutput* GetGreenOutput() const { return m_greenOutput; }
   IOutput* GetBlueOutput() const { return m_blueOutput; }

   RGBColor GetResultingData();

   virtual const std::string& GetOutputNameBlue() const override { return m_outputNameBlue; }
   virtual void SetOutputNameBlue(const std::string& name) override { m_outputNameBlue = name; }
   virtual const std::string& GetOutputNameGreen() const override { return m_outputNameGreen; }
   virtual void SetOutputNameGreen(const std::string& name) override { m_outputNameGreen = name; }
   virtual const std::string& GetOutputNameRed() const override { return m_outputNameRed; }
   virtual void SetOutputNameRed(const std::string& name) override { m_outputNameRed = name; }

private:
   LayerDictionary<RGBAColor> m_layers;

   std::string m_outputNameRed;
   std::string m_outputNameGreen;
   std::string m_outputNameBlue;

   IOutput* m_redOutput;
   IOutput* m_greenOutput;
   IOutput* m_blueOutput;
};

}