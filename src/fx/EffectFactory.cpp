#include "EffectFactory.h"

#include "analogtoyfx/AnalogToyValueEffect.h"
#include "conditionfx/TableElementConditionEffect.h"
#include "listfx/ListEffect.h"
#include "nullfx/NullEffect.h"
#include "rgbafx/RGBAColorEffect.h"
#include "timmedfx/BlinkEffect.h"
#include "timmedfx/DelayEffect.h"
#include "timmedfx/DurationEffect.h"
#include "timmedfx/ExtendDurationEffect.h"
#include "timmedfx/FadeEffect.h"
#include "timmedfx/MaxDurationEffect.h"
#include "timmedfx/MinDurationEffect.h"
#include "valuefx/ValueInvertEffect.h"
#include "valuefx/ValueMapFullRangeEffect.h"
#include "matrixfx/AnalogAlphaMatrixBitmapAnimationEffect.h"
#include "matrixfx/AnalogAlphaMatrixBitmapEffect.h"
#include "matrixfx/AnalogAlphaMatrixFlickerEffect.h"
#include "matrixfx/AnalogAlphaMatrixShiftEffect.h"
#include "matrixfx/AnalogAlphaMatrixValueEffect.h"
#include "matrixfx/RGBAMatrixBitmapAnimationEffect.h"
#include "matrixfx/RGBAMatrixBitmapEffect.h"
#include "matrixfx/RGBAMatrixColorEffect.h"
#include "matrixfx/RGBAMatrixColorScaleBitmapAnimationEffect.h"
#include "matrixfx/RGBAMatrixColorScaleBitmapEffect.h"
#include "matrixfx/RGBAMatrixColorScaleShapeEffect.h"
#include "matrixfx/RGBAMatrixFlickerEffect.h"
#include "matrixfx/RGBAMatrixPlasmaEffect.h"
#include "matrixfx/RGBAMatrixShapeEffect.h"
#include "matrixfx/RGBAMatrixShiftEffect.h"

#include <algorithm>

namespace DOF
{

EffectFactory& EffectFactory::GetInstance()
{
   static EffectFactory instance;
   return instance;
}

void EffectFactory::RegisterEffect(const std::string& typeName, EffectCreator creator) { m_creators[typeName] = creator; }

IEffect* EffectFactory::CreateEffect(const std::string& typeName) const
{
   auto it = m_creators.find(typeName);
   if (it != m_creators.end())
   {
      return it->second();
   }
   return nullptr;
}

bool EffectFactory::IsEffectRegistered(const std::string& typeName) const { return m_creators.find(typeName) != m_creators.end(); }

std::vector<std::string> EffectFactory::GetRegisteredTypes() const
{
   std::vector<std::string> types;
   types.reserve(m_creators.size());

   for (const auto& pair : m_creators)
   {
      types.push_back(pair.first);
   }

   std::sort(types.begin(), types.end());
   return types;
}

REGISTER_EFFECT(AnalogToyValueEffect, "AnalogToyValueEffect")
REGISTER_EFFECT(TableElementConditionEffect, "TableElementConditionEffect")
REGISTER_EFFECT(ListEffect, "ListEffect")
REGISTER_EFFECT(NullEffect, "NullEffect")
REGISTER_EFFECT(RGBAColorEffect, "RGBAColorEffect")
REGISTER_EFFECT(BlinkEffect, "BlinkEffect")
REGISTER_EFFECT(DelayEffect, "DelayEffect")
REGISTER_EFFECT(DurationEffect, "DurationEffect")
REGISTER_EFFECT(ExtendDurationEffect, "ExtendDurationEffect")
REGISTER_EFFECT(FadeEffect, "FadeEffect")
REGISTER_EFFECT(MaxDurationEffect, "MaxDurationEffect")
REGISTER_EFFECT(MinDurationEffect, "MinDurationEffect")
REGISTER_EFFECT(ValueInvertEffect, "ValueInvertEffect")
REGISTER_EFFECT(ValueMapFullRangeEffect, "ValueMapFullRangeEffect")

REGISTER_MATRIX_EFFECT(AnalogAlphaMatrixBitmapAnimationEffect, "AnalogAlphaMatrixBitmapAnimationEffect")
REGISTER_MATRIX_EFFECT(AnalogAlphaMatrixBitmapEffect, "AnalogAlphaMatrixBitmapEffect")
REGISTER_MATRIX_EFFECT(AnalogAlphaMatrixFlickerEffect, "AnalogAlphaMatrixFlickerEffect")
REGISTER_MATRIX_EFFECT(AnalogAlphaMatrixShiftEffect, "AnalogAlphaMatrixShiftEffect")
REGISTER_MATRIX_EFFECT(AnalogAlphaMatrixValueEffect, "AnalogAlphaMatrixValueEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixBitmapAnimationEffect, "RGBAMatrixBitmapAnimationEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixBitmapEffect, "RGBAMatrixBitmapEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixColorEffect, "RGBAMatrixColorEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixColorScaleBitmapAnimationEffect, "RGBAMatrixColorScaleBitmapAnimationEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixColorScaleBitmapEffect, "RGBAMatrixColorScaleBitmapEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixColorScaleShapeEffect, "RGBAMatrixColorScaleShapeEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixFlickerEffect, "RGBAMatrixFlickerEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixPlasmaEffect, "RGBAMatrixPlasmaEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixShapeEffect, "RGBAMatrixShapeEffect")
REGISTER_MATRIX_EFFECT(RGBAMatrixShiftEffect, "RGBAMatrixShiftEffect")

}