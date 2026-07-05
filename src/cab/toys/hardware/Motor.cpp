#include "Motor.h"
#include "../../Cabinet.h"
#include "../../../pinballsupport/AlarmHandler.h"
#include "../../out/IOutput.h"
#include "../../../Log.h"
#include "../../../general/MathExtensions.h"
#include <algorithm>
#include <functional>

namespace DOF
{

Motor::Motor()
   : m_maxRunTimeMs(300000)
   , m_kickstartPower(255)
   , m_kickstartDurationMs(100)
   , m_minPower(1)
   , m_maxPower(255)
   , m_currentMotorPower(0)
   , m_targetMotorPower(0)
   , m_kickstartActive(false)
   , m_turnedOffAfterMaxRunTime(false)
   , m_alarmHandler(nullptr)
{
}

Motor::~Motor() { }

void Motor::Init(Cabinet* cabinet)
{
   AnalogAlphaToy::Init(cabinet);

   if (cabinet != nullptr)
   {
      m_alarmHandler = cabinet->GetAlarms();
   }
}

void Motor::Finish()
{
   if (m_alarmHandler != nullptr)
   {
      m_alarmHandler->UnregisterAlarm(Action(this, &Motor::KickStartEnd));
      m_alarmHandler->UnregisterAlarm(Action(this, &Motor::MaxRunTimeMotorStop));
      m_alarmHandler = nullptr;
   }

   AnalogAlphaToy::Finish();
}

void Motor::UpdateOutputs()
{
   IOutput* output = GetOutput();
   if (output == nullptr)
      return;

   int p = GetCurrentValue().GetValue();
   p = MathExtensions::Limit(p, 0, 255);

   if (p != 0)
   {
      int powerRange = (m_maxPower >= m_minPower) ? (m_maxPower - m_minPower) : (m_minPower - m_maxPower);
      p = static_cast<int>((static_cast<double>(powerRange) / 255.0 * p) + m_minPower);
      p = MathExtensions::Limit(p, m_minPower, m_maxPower);
   }

   if (p == 0)
      m_turnedOffAfterMaxRunTime = false;

   if (!m_turnedOffAfterMaxRunTime)
   {
      if (m_currentMotorPower == 0)
      {
         if (p > 0)
         {
            if (m_kickstartDurationMs > 0 && m_kickstartPower > 0 && p <= m_kickstartPower)
            {
               m_targetMotorPower = p;

               if (!m_kickstartActive)
               {
                  m_currentMotorPower = m_kickstartPower;
                  output->SetOutput(static_cast<uint8_t>(m_currentMotorPower));
                  m_kickstartActive = true;

                  if (m_alarmHandler != nullptr)
                  {
                     m_alarmHandler->RegisterAlarm(m_kickstartDurationMs, Action(this, &Motor::KickStartEnd));
                  }
               }
            }
            else
            {
               m_currentMotorPower = p;
               m_targetMotorPower = p;
               output->SetOutput(static_cast<uint8_t>(p));
               m_kickstartActive = false;
            }

            if (m_maxRunTimeMs > 0 && m_alarmHandler != nullptr)
            {
               m_alarmHandler->RegisterAlarm(m_maxRunTimeMs, Action(this, &Motor::MaxRunTimeMotorStop));
            }
         }
      }
      else if (m_kickstartActive)
      {
         if (p > 0)
         {
            m_targetMotorPower = p;
         }
         else
         {
            if (m_alarmHandler != nullptr)
            {
               m_alarmHandler->UnregisterAlarm(Action(this, &Motor::KickStartEnd));
               m_alarmHandler->UnregisterAlarm(Action(this, &Motor::MaxRunTimeMotorStop));
            }
            m_targetMotorPower = 0;
            m_currentMotorPower = 0;
            output->SetOutput(0);
         }
      }
      else
      {
         if (p == 0)
         {
            if (m_alarmHandler != nullptr)
            {
               m_alarmHandler->UnregisterAlarm(Action(this, &Motor::KickStartEnd));
               m_alarmHandler->UnregisterAlarm(Action(this, &Motor::MaxRunTimeMotorStop));
            }
            m_targetMotorPower = 0;
            m_currentMotorPower = 0;
            output->SetOutput(0);
         }
         else if (p != m_currentMotorPower)
         {
            m_currentMotorPower = p;
            m_targetMotorPower = p;
            output->SetOutput(static_cast<uint8_t>(p));
         }
      }
   }
}

void Motor::MaxRunTimeMotorStop()
{
   if (m_alarmHandler != nullptr)
   {
      m_alarmHandler->UnregisterAlarm(Action(this, &Motor::KickStartEnd));
   }

   m_kickstartActive = false;
   m_currentMotorPower = 0;
   m_targetMotorPower = 0;
   m_turnedOffAfterMaxRunTime = true;

   IOutput* output = GetOutput();
   if (output != nullptr)
   {
      output->SetOutput(0);
   }
}

void Motor::KickStartEnd()
{
   m_kickstartActive = false;
   m_currentMotorPower = m_targetMotorPower;

   IOutput* output = GetOutput();
   if (output != nullptr)
   {
      output->SetOutput(static_cast<uint8_t>(m_currentMotorPower));
   }
}

void Motor::SetMaxRunTimeMs(int value) { m_maxRunTimeMs = MathExtensions::Limit(value, 0, INT_MAX); }

void Motor::SetKickstartPower(int value) { m_kickstartPower = MathExtensions::Limit(value, 0, 255); }

void Motor::SetKickstartDurationMs(int value) { m_kickstartDurationMs = MathExtensions::Limit(value, 0, 5000); }

void Motor::SetMinPower(int value) { m_minPower = MathExtensions::Limit(value, 0, 255); }

void Motor::SetMaxPower(int value) { m_maxPower = MathExtensions::Limit(value, 0, 255); }

}
