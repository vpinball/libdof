#pragma once

#include "MatrixEffectBase.h"
#include "../../general/MathExtensions.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../pinballsupport/Action.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace DOF
{

class TableElementData;

template <typename MatrixElementType> class MatrixPlasmaEffectBase : public MatrixEffectBase<MatrixElementType>
{
public:
   MatrixPlasmaEffectBase();
   virtual ~MatrixPlasmaEffectBase() = default;

   int GetPlasmaSpeed() const { return m_plasmaSpeed; }
   void SetPlasmaSpeed(int value) { m_plasmaSpeed = MathExtensions::Limit(value, 1, INT_MAX); }
   int GetPlasmaScale() const { return m_plasmaScale; }
   void SetPlasmaScale(int value) { m_plasmaScale = value; }
   int GetPlasmaDensity() const { return m_plasmaDensity; }
   void SetPlasmaDensity(int value) { m_plasmaDensity = value; }
   virtual void Trigger(TableElementData* tableElementData) override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual MatrixElementType GetEffectValue(int triggerValue, double time, double value, double x, double y) = 0;

private:
   void DoPlasma();
   void DrawFrame();
   void ClearFrame();
   void PrecalcTimeValues(double time);
   void PrecalcXValues(double x, double time);
   double CalcPositionValue(double x, double y, double time);

   static const int RefreshIntervalMs = 30;

   int m_plasmaSpeed;
   int m_plasmaScale;
   int m_plasmaDensity;
   int m_currentTriggerValue;
   bool m_active;
   double m_time;
   Action m_plasmaCallback;

   double m_cosTime;
   double m_sinTimeDiv11767Mult05;
   double m_cosTimeDiv1833371Mult05;
   double m_xWaveValue1;
   double m_xWaveValue2;
};


template <typename MatrixElementType>
MatrixPlasmaEffectBase<MatrixElementType>::MatrixPlasmaEffectBase()
   : m_plasmaSpeed(100)
   , m_plasmaScale(100)
   , m_plasmaDensity(100)
   , m_currentTriggerValue(0)
   , m_active(false)
   , m_time(0.0)
   , m_cosTime(0.0)
   , m_sinTimeDiv11767Mult05(0.0)
   , m_cosTimeDiv1833371Mult05(0.0)
   , m_xWaveValue1(0.0)
   , m_xWaveValue2(0.0)
{
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::Trigger(TableElementData* tableElementData)
{
   if (this->m_matrixLayer != nullptr)
   {
      m_currentTriggerValue = tableElementData->m_value;
      if (m_currentTriggerValue > 0 && !m_active)
         DoPlasma();
   }
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::DoPlasma()
{
   int v = MathExtensions::Limit(m_currentTriggerValue, 0, 255);
   if (v > 0)
   {
      if (!m_active)
      {
         if (!m_plasmaCallback)
            m_plasmaCallback = Action(this, &MatrixPlasmaEffectBase<MatrixElementType>::DoPlasma);
         this->m_table->GetPinball()->GetAlarms()->RegisterIntervalAlarm(RefreshIntervalMs, m_plasmaCallback);
         m_active = true;
      }
      DrawFrame();
   }
   else
   {
      this->m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_plasmaCallback);
      m_active = false;
      ClearFrame();
   }
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::DrawFrame()
{
   int f = (this->GetFadeMode() == FadeModeEnum::OnOff ? (m_currentTriggerValue > 0 ? 255 : 0) : MathExtensions::Limit(m_currentTriggerValue, 0, 255));

   m_time += ((double)m_plasmaSpeed / 2000);
   int w = this->GetAreaWidth();
   int h = this->GetAreaHeight();

   double sx;
   double sy;

   if (w >= h)
   {
      sx = (double)m_plasmaScale / 100 / w;
      sy = sx;
   }
   else
   {
      sy = (double)m_plasmaScale / 100 / h;
      sx = sy;
   }

   PrecalcTimeValues(m_time);

   for (int x = 0; x < w; x++)
   {
      double xx = sx * x;
      PrecalcXValues(xx, m_time);

      for (int y = 0; y < h; y++)
      {
         double yy = (double)sy * y;

         double v = std::max(0.0, std::min(CalcPositionValue(xx, yy, m_time), 1.0));

         int idx = (this->m_areaTop + y) * this->m_matrix->GetWidth() + (this->m_areaLeft + x);
         this->m_matrixLayer[idx] = GetEffectValue(f, m_time, v, xx, yy);
      }
   }
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::ClearFrame()
{
   int w = this->GetAreaWidth();
   int h = this->GetAreaHeight();

   for (int y = 0; y < h; y++)
   {
      for (int x = 0; x < w; x++)
      {
         int idx = (this->m_areaTop + y) * this->m_matrix->GetWidth() + (this->m_areaLeft + x);
         this->m_matrixLayer[idx] = GetEffectValue(0, 0, 0, 0, 0);
      }
   }
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::PrecalcTimeValues(double time)
{
   m_cosTime = std::cos(time);
   m_sinTimeDiv11767Mult05 = 0.5 * std::sin(time / 1.1767);
   m_cosTimeDiv1833371Mult05 = 0.5 * std::cos(time / 1.833371);
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::PrecalcXValues(double x, double time)
{
   m_xWaveValue1 = std::sin(x * M_PI * (m_plasmaDensity / 28) + time);
   m_xWaveValue2 = x * std::sin(time / 2.567);
}

template <typename MatrixElementType> double MatrixPlasmaEffectBase<MatrixElementType>::CalcPositionValue(double x, double y, double time)
{
   double v = m_xWaveValue1;

   v += std::sin(M_PI * (m_plasmaDensity / 28) * (m_xWaveValue2 + y * m_cosTime) + time);

   double cx = x + m_sinTimeDiv11767Mult05;
   double cy = y + m_cosTimeDiv1833371Mult05;
   v += std::sin(std::sqrt((M_PI * (m_plasmaDensity / 56)) * (M_PI * (m_plasmaDensity / 56)) * (cx * cx + cy * cy) + 1) + time);
   v = ((v + 3) / 6.0);

   return v;
}

template <typename MatrixElementType> void MatrixPlasmaEffectBase<MatrixElementType>::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixEffectBase<MatrixElementType>::DeserializeFromXml(element);

   int value;
   if (EffectBase::ReadXmlInt(element, "PlasmaSpeed", value))
      this->SetPlasmaSpeed(value);
   if (EffectBase::ReadXmlInt(element, "PlasmaScale", value))
      this->SetPlasmaScale(value);
   if (EffectBase::ReadXmlInt(element, "PlasmaDensity", value))
      this->SetPlasmaDensity(value);
}

}
