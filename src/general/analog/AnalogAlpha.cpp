#include "AnalogAlpha.h"

namespace DOF
{

AnalogAlpha::AnalogAlpha()
   : m_value(0)
   , m_alpha(0)
{
}

AnalogAlpha::AnalogAlpha(int value, int alpha)
   : m_value(std::clamp(value, 0, 255))
   , m_alpha(std::clamp(alpha, 0, 255))
{
}

bool AnalogAlpha::operator==(const AnalogAlpha& other) const { return m_value == other.m_value && m_alpha == other.m_alpha; }

}