#pragma once

#include "MatrixEffectBase.h"
#include "MatrixShiftDirectionEnum.h"
#include "../FadeModeEnum.h"
#include "../../general/MathExtensions.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../pinballsupport/Action.h"
#include <cmath>
#include <map>
#include <vector>

namespace DOF
{

class TableElementData;

template <typename MatrixElementType> class MatrixShiftEffectBase : public MatrixEffectBase<MatrixElementType>
{
public:
   MatrixShiftEffectBase();
   virtual ~MatrixShiftEffectBase() = default;

   MatrixShiftDirectionEnum GetShiftDirection() const { return m_shiftDirection; }
   void SetShiftDirection(MatrixShiftDirectionEnum value) { m_shiftDirection = value; }
   float GetShiftSpeed() const { return m_shiftSpeed; }
   void SetShiftSpeed(float value) { m_shiftSpeed = MathExtensions::Limit(value, 1.0f, 10000.0f); }
   float GetShiftAcceleration() const { return m_shiftAcceleration; }
   void SetShiftAcceleration(float value) { m_shiftAcceleration = value; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual MatrixElementType GetEffectValue(int triggerValue) = 0;

private:
   void BuildStep2ElementTable();
   void DoStep();

   static const int RefreshIntervalMs = 30;

   MatrixShiftDirectionEnum m_shiftDirection;
   float m_shiftSpeed;
   float m_shiftAcceleration;

   std::vector<float> m_step2Element;
   std::map<int, int> m_triggerValueBuffer;
   int m_lastTriggerValue;
   int m_lastDiscardedValue;
   int m_currentStep;
   bool m_active;
   Action m_stepCallback;
};


template <typename MatrixElementType>
MatrixShiftEffectBase<MatrixElementType>::MatrixShiftEffectBase()
   : m_shiftDirection(MatrixShiftDirectionEnum::Right)
   , m_shiftSpeed(200.0f)
   , m_shiftAcceleration(0.0f)
   , m_lastTriggerValue(0)
   , m_lastDiscardedValue(0)
   , m_currentStep(0)
   , m_active(false)
   , m_stepCallback(this, &MatrixShiftEffectBase<MatrixElementType>::DoStep)
{
}

template <typename MatrixElementType> void MatrixShiftEffectBase<MatrixElementType>::BuildStep2ElementTable()
{
   std::vector<float> stepList;

   float numberOfElements
      = (m_shiftDirection == MatrixShiftDirectionEnum::Left || m_shiftDirection == MatrixShiftDirectionEnum::Right) ? (float)this->GetAreaWidth() : (float)this->GetAreaHeight();
   float position = 0.0f;
   float speed = numberOfElements / 100.0f * (m_shiftSpeed / (1000 / RefreshIntervalMs));
   float acceleration = numberOfElements / 100.0f * (m_shiftAcceleration / (1000 / RefreshIntervalMs));

   while (position <= numberOfElements)
   {
      stepList.push_back(MathExtensions::Limit(position, 0.0f, numberOfElements));
      position += speed;
      speed = MathExtensions::Limit(speed + acceleration, numberOfElements / 100.0f * (float)(1 / (1000 / RefreshIntervalMs)), 10000.0f);
   }
   stepList.push_back(MathExtensions::Limit(position, 0.0f, numberOfElements));

   m_step2Element = stepList;
}

template <typename MatrixElementType> void MatrixShiftEffectBase<MatrixElementType>::DoStep()
{
   if (!m_active)
   {
      this->m_table->GetPinball()->GetAlarms()->RegisterIntervalAlarm(RefreshIntervalMs, m_stepCallback);
      m_active = true;
   }

   int numberOfElements = (m_shiftDirection == MatrixShiftDirectionEnum::Left || m_shiftDirection == MatrixShiftDirectionEnum::Right) ? this->GetAreaWidth() : this->GetAreaHeight();

   float fromElementNr = (float)numberOfElements;
   float toElementNr = 0.0f;
   std::vector<float> value(numberOfElements + 1, 0.0f);

   int lastValue = m_lastDiscardedValue;

   int toNr;
   for (auto& kv : m_triggerValueBuffer)
   {
      toElementNr = m_step2Element[(m_currentStep - kv.first)];

      if ((int)fromElementNr == (int)toElementNr)
         value[(int)fromElementNr] += (fromElementNr - toElementNr) * (float)lastValue;
      else
      {
         if (fromElementNr != (float)(int)fromElementNr)
            value[(int)fromElementNr] += (fromElementNr - (float)(int)fromElementNr) * (float)lastValue;

         toNr = (int)std::ceil(toElementNr);
         for (int i = (int)fromElementNr - 1; i >= toNr; i--)
            value[i] = (float)lastValue;
         if (toElementNr != (float)(int)toElementNr)
            value[(int)toElementNr] += ((float)(int)toElementNr + 1.0f - toElementNr) * (float)lastValue;
      }
      fromElementNr = toElementNr;
      lastValue = kv.second;
   }
   toElementNr = 0;
   if (fromElementNr != toElementNr)
   {
      if (fromElementNr != (float)(int)fromElementNr && (int)fromElementNr < (int)this->GetWidth() - 1)
         value[(int)fromElementNr] += (fromElementNr - (float)(int)fromElementNr) * (float)lastValue;

      toNr = MathExtensions::Limit((int)std::ceil(toElementNr), 0, 2147483647);
      for (int i = (int)fromElementNr - 1; i >= toNr; i--)
         value[i] = (float)lastValue;
      if (toElementNr != (float)(int)toElementNr)
         value[(int)toElementNr] += ((float)(int)toElementNr + 1.0f - toElementNr) * (float)lastValue;
   }

   switch (m_shiftDirection)
   {
   case MatrixShiftDirectionEnum::Right:
      for (int i = 0; i < numberOfElements; i++)
      {
         int v = MathExtensions::Limit((int)value[i], 0, 255);
         if (v > 0 && this->GetFadeMode() == FadeModeEnum::OnOff)
            v = 255;
         MatrixElementType d = GetEffectValue(v);

         for (int y = this->m_areaTop; y <= this->m_areaBottom; y++)
         {
            int idx = y * this->m_matrix->GetWidth() + (this->m_areaLeft + i);
            this->m_matrixLayer[idx] = d;
         }
      }
      break;
   case MatrixShiftDirectionEnum::Down:
      for (int i = 0; i < numberOfElements; i++)
      {
         int v = MathExtensions::Limit((int)value[i], 0, 255);
         if (v > 0 && this->GetFadeMode() == FadeModeEnum::OnOff)
            v = 255;
         MatrixElementType d = GetEffectValue(v);

         for (int x = this->m_areaLeft; x <= this->m_areaRight; x++)
         {
            int idx = (this->m_areaTop + i) * this->m_matrix->GetWidth() + x;
            this->m_matrixLayer[idx] = d;
         }
      }
      break;
   case MatrixShiftDirectionEnum::Up:
      for (int i = 0; i < numberOfElements; i++)
      {
         int v = MathExtensions::Limit((int)value[i], 0, 255);
         if (v > 0 && this->GetFadeMode() == FadeModeEnum::OnOff)
            v = 255;
         MatrixElementType d = GetEffectValue(v);

         for (int x = this->m_areaLeft; x <= this->m_areaRight; x++)
         {
            int idx = (this->m_areaBottom - i) * this->m_matrix->GetWidth() + x;
            this->m_matrixLayer[idx] = d;
         }
      }
      break;
   case MatrixShiftDirectionEnum::Left:
   default:
      for (int i = 0; i < numberOfElements; i++)
      {
         int v = MathExtensions::Limit((int)value[i], 0, 255);
         if (v > 0 && this->GetFadeMode() == FadeModeEnum::OnOff)
            v = 255;
         MatrixElementType d = GetEffectValue(v);

         for (int y = this->m_areaTop; y <= this->m_areaBottom; y++)
         {
            int idx = y * this->m_matrix->GetWidth() + (this->m_areaRight - i);
            this->m_matrixLayer[idx] = d;
         }
      }
      break;
   }

   int dropKey = m_currentStep - ((int)m_step2Element.size() - 1);
   auto it = m_triggerValueBuffer.find(dropKey);
   if (it != m_triggerValueBuffer.end())
   {
      m_lastDiscardedValue = it->second;
      m_triggerValueBuffer.erase(it);
   }

   if (!m_triggerValueBuffer.empty() || m_lastDiscardedValue != 0)
      m_currentStep++;
   else
   {
      this->m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_stepCallback);
      m_lastDiscardedValue = 0;
      m_currentStep = 0;
      m_active = false;
   }
}

template <typename MatrixElementType> void MatrixShiftEffectBase<MatrixElementType>::Trigger(TableElementData* tableElementData)
{
   if (m_lastTriggerValue != tableElementData->m_value && this->m_matrixLayer != nullptr)
   {
      m_lastTriggerValue = tableElementData->m_value;

      m_triggerValueBuffer[m_currentStep] = m_lastTriggerValue;

      if (!m_active)
         DoStep();
   }
}

template <typename MatrixElementType> void MatrixShiftEffectBase<MatrixElementType>::Init(Table* table)
{
   MatrixEffectBase<MatrixElementType>::Init(table);
   BuildStep2ElementTable();
}

template <typename MatrixElementType> void MatrixShiftEffectBase<MatrixElementType>::Finish()
{
   try
   {
      this->m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_stepCallback);
   }
   catch (...)
   {
   }
   MatrixEffectBase<MatrixElementType>::Finish();
}

template <typename MatrixElementType> void MatrixShiftEffectBase<MatrixElementType>::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixEffectBase<MatrixElementType>::DeserializeFromXml(element);

   std::string text;
   float floatValue;
   if (EffectBase::ReadXmlString(element, "ShiftDirection", text))
   {
      if (text == "Left")
         this->SetShiftDirection(MatrixShiftDirectionEnum::Left);
      else if (text == "Right")
         this->SetShiftDirection(MatrixShiftDirectionEnum::Right);
      else if (text == "Up")
         this->SetShiftDirection(MatrixShiftDirectionEnum::Up);
      else if (text == "Down")
         this->SetShiftDirection(MatrixShiftDirectionEnum::Down);
      else
         EffectBase::ThrowInvalidXmlValue("ShiftDirection", text);
   }
   if (EffectBase::ReadXmlFloat(element, "ShiftSpeed", floatValue))
      this->SetShiftSpeed(floatValue);
   if (EffectBase::ReadXmlFloat(element, "ShiftAcceleration", floatValue))
      this->SetShiftAcceleration(floatValue);
}

}