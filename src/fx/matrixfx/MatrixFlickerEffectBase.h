#pragma once

#include "MatrixEffectBase.h"
#include "../RetriggerBehaviourEnum.h"
#include "../../general/MathExtensions.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../pinballsupport/Action.h"
#include <random>
#include <chrono>
#include <functional>

namespace DOF
{

class TableElementData;

template <typename MatrixElementType> class MatrixFlickerEffectBase : public MatrixEffectBase<MatrixElementType>
{
public:
   MatrixFlickerEffectBase();
   virtual ~MatrixFlickerEffectBase() = default;

   int GetDensity() const { return m_density; }
   void SetDensity(int value) { m_density = MathExtensions::Limit(value, 0, 100); }
   int GetMinFlickerDurationMs() const { return m_minFlickerDurationMs; }
   void SetMinFlickerDurationMs(int value) { m_minFlickerDurationMs = MathExtensions::Limit(value, 1, INT_MAX); }
   int GetMaxFlickerDurationMs() const { return m_maxFlickerDurationMs; }
   void SetMaxFlickerDurationMs(int value) { m_maxFlickerDurationMs = MathExtensions::Limit(value, 1, INT_MAX); }
   int GetFlickerFadeUpDurationMs() const { return m_flickerFadeUpDurationMs; }
   void SetFlickerFadeUpDurationMs(int value) { m_flickerFadeUpDurationMs = MathExtensions::Limit(value, 1, INT_MAX); }
   int GetFlickerFadeDownDurationMs() const { return m_flickerFadeDownDurationMs; }
   void SetFlickerFadeDownDurationMs(int value) { m_flickerFadeDownDurationMs = MathExtensions::Limit(value, 1, INT_MAX); }
   RetriggerBehaviourEnum GetRetriggerBehaviour() const { return m_retriggerBehaviour; }
   void SetRetriggerBehaviour(RetriggerBehaviourEnum value) { m_retriggerBehaviour = value; }
   virtual void Init(Table* table) override;
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   virtual MatrixElementType GetEffectValue(int triggerValue) = 0;

private:
   void BuildFlickerObjects();
   void DoFlicker();

   int m_density;
   int m_minFlickerDurationMs;
   int m_maxFlickerDurationMs;
   int m_flickerFadeUpDurationMs;
   int m_flickerFadeDownDurationMs;
   RetriggerBehaviourEnum m_retriggerBehaviour;
   bool m_active;
   int m_currentValue;
   TableElementData* m_flickerTableElementData;
   std::mt19937 m_randomGenerator;
   Action m_intervalAlarmCallback;

   struct FlickerObject
   {
      int X;
      int Y;
      int DurationMs;
      std::chrono::steady_clock::time_point StartTimestamp;
   };

   std::vector<FlickerObject> m_activeFlickerObjects;
   std::vector<FlickerObject> m_inactiveFlickerObjects;
};

template <typename MatrixElementType>
MatrixFlickerEffectBase<MatrixElementType>::MatrixFlickerEffectBase()
   : m_density(10)
   , m_minFlickerDurationMs(60)
   , m_maxFlickerDurationMs(150)
   , m_flickerFadeUpDurationMs(0)
   , m_flickerFadeDownDurationMs(0)
   , m_retriggerBehaviour(RetriggerBehaviourEnum::Restart)
   , m_active(false)
   , m_currentValue(0)
   , m_flickerTableElementData(nullptr)
   , m_randomGenerator(std::random_device { }())
   , m_intervalAlarmCallback()
{
}

template <typename MatrixElementType> void MatrixFlickerEffectBase<MatrixElementType>::Init(Table* table)
{
   MatrixEffectBase<MatrixElementType>::Init(table);
   BuildFlickerObjects();
}

template <typename MatrixElementType> void MatrixFlickerEffectBase<MatrixElementType>::BuildFlickerObjects()
{
   m_activeFlickerObjects.clear();
   m_inactiveFlickerObjects.clear();

   for (int Y = this->m_areaTop; Y <= this->m_areaBottom; Y++)
   {
      for (int X = this->m_areaLeft; X <= this->m_areaRight; X++)
      {
         FlickerObject fo;
         fo.X = X;
         fo.Y = Y;
         fo.DurationMs = 0;
         fo.StartTimestamp = std::chrono::steady_clock::now();
         m_inactiveFlickerObjects.push_back(fo);
      }
   }
}

template <typename MatrixElementType> void MatrixFlickerEffectBase<MatrixElementType>::Trigger(TableElementData* tableElementData)
{
   if (this->m_matrixLayer != nullptr)
   {
      m_currentValue = tableElementData->m_value;
      if (m_currentValue > 0 && !m_active)
      {
         DoFlicker();
      }
   }
}

template <typename MatrixElementType> void MatrixFlickerEffectBase<MatrixElementType>::DoFlicker()
{
   MatrixElementType i = GetEffectValue(0);
   int v = MathExtensions::Limit(m_currentValue, 0, 255);

   if (v > 0)
   {
      if (!m_active)
      {
         if (!m_intervalAlarmCallback)
         {
            m_intervalAlarmCallback = Action(this, &MatrixFlickerEffectBase<MatrixElementType>::DoFlicker);
         }
         this->m_table->GetPinball()->GetAlarms()->RegisterIntervalAlarm(30, m_intervalAlarmCallback);
         m_active = true;
      }

      if (v > 0 && this->GetFadeMode() == FadeModeEnum::OnOff)
         v = 255;

      int numberOfLeds = (this->m_areaRight - this->m_areaLeft + 1) * (this->m_areaBottom - this->m_areaTop + 1);

      int flickerLeds = MathExtensions::Limit((int)((double)numberOfLeds / 100.0 * m_density), 1, numberOfLeds);

      int min = m_minFlickerDurationMs;
      int max = m_maxFlickerDurationMs;
      if (max < min)
      {
         int tmp = min;
         min = max;
         max = tmp;
      }


      while (m_activeFlickerObjects.size() < static_cast<size_t>(flickerLeds) && m_inactiveFlickerObjects.size() > 0)
      {
         std::uniform_int_distribution<int> indexDist(0, static_cast<int>(m_inactiveFlickerObjects.size() - 1));
         int index = indexDist(m_randomGenerator);
         FlickerObject fo = m_inactiveFlickerObjects[index];
         m_inactiveFlickerObjects.erase(m_inactiveFlickerObjects.begin() + index);

         fo.StartTimestamp = std::chrono::steady_clock::now();
         std::uniform_int_distribution<int> durationDist(min, max - 1);
         fo.DurationMs = durationDist(m_randomGenerator) + m_flickerFadeDownDurationMs;
         m_activeFlickerObjects.push_back(fo);
      }

      for (int i = static_cast<int>(m_activeFlickerObjects.size() - 1); i >= 0; i--)
      {
         FlickerObject& fo = m_activeFlickerObjects[i];

         int fv;
         int ageMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - fo.StartTimestamp).count());


         if (ageMs > fo.DurationMs + m_flickerFadeDownDurationMs)
         {
            if (ageMs > (fo.DurationMs + m_flickerFadeDownDurationMs) * 2 || (std::uniform_real_distribution<double>(0.0, 1.0)(m_randomGenerator) > 0.5))
            {
               if (this->m_matrixLayer != nullptr)
               {
                  int idx = fo.Y * this->m_matrix->GetWidth() + fo.X;
                  this->m_matrixLayer[idx] = GetEffectValue(0);
               }

               m_inactiveFlickerObjects.push_back(fo);
               m_activeFlickerObjects.erase(m_activeFlickerObjects.begin() + i);
               continue;
            }
            fv = 0;
         }
         else if (m_flickerFadeUpDurationMs > 0 && ageMs < m_flickerFadeUpDurationMs && ageMs < fo.DurationMs)
         {
            fv = (int)((double)v / m_flickerFadeUpDurationMs * ageMs);
         }
         else if (ageMs > fo.DurationMs && m_flickerFadeDownDurationMs > 0)
         {
            if (fo.DurationMs < m_flickerFadeUpDurationMs)
            {
               fv = (int)((double)v / m_flickerFadeUpDurationMs * fo.DurationMs);
            }
            else
            {
               fv = v;
            }
            fv = fv - (int)((double)fv / m_flickerFadeDownDurationMs * (ageMs - fo.DurationMs));
         }
         else
         {
            fv = v;
         }

         fv = MathExtensions::Limit(fv, 0, 255);

         if (this->m_matrixLayer != nullptr)
         {
            int idx = fo.Y * this->m_matrix->GetWidth() + fo.X;
            this->m_matrixLayer[idx] = GetEffectValue(fv);
         }
      }
   }
   else
   {
      for (auto& fo : m_activeFlickerObjects)
      {
         if (this->m_matrixLayer != nullptr)
         {
            int idx = fo.Y * this->m_matrix->GetWidth() + fo.X;
            this->m_matrixLayer[idx] = i;
         }
      }

      m_inactiveFlickerObjects.insert(m_inactiveFlickerObjects.end(), m_activeFlickerObjects.begin(), m_activeFlickerObjects.end());
      m_activeFlickerObjects.clear();

      if (m_intervalAlarmCallback)
      {
         this->m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(m_intervalAlarmCallback);
         m_intervalAlarmCallback = Action();
      }
      m_active = false;
   }
}

template <typename MatrixElementType> void MatrixFlickerEffectBase<MatrixElementType>::Finish()
{
   try
   {
   }
   catch (...)
   {
   }

   if (m_active)
   {
      m_currentValue = 0;
      DoFlicker();
   }

   m_activeFlickerObjects.clear();
   m_inactiveFlickerObjects.clear();

   m_active = false;
   m_flickerTableElementData = nullptr;
   MatrixEffectBase<MatrixElementType>::Finish();
}

template <typename MatrixElementType> void MatrixFlickerEffectBase<MatrixElementType>::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixEffectBase<MatrixElementType>::DeserializeFromXml(element);

   int value;
   if (EffectBase::ReadXmlInt(element, "Density", value))
      this->SetDensity(value);
   if (EffectBase::ReadXmlInt(element, "MinFlickerDurationMs", value))
      this->SetMinFlickerDurationMs(value);
   if (EffectBase::ReadXmlInt(element, "MaxFlickerDurationMs", value))
      this->SetMaxFlickerDurationMs(value);
   if (EffectBase::ReadXmlInt(element, "FlickerFadeUpDurationMs", value))
      this->SetFlickerFadeUpDurationMs(value);
   if (EffectBase::ReadXmlInt(element, "FlickerFadeDownDurationMs", value))
      this->SetFlickerFadeDownDurationMs(value);
}

}