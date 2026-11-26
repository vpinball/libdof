#include "RGBAMatrixColorScaleShapeEffect.h"
#include "RGBAMatrixBitmapEffect.h"
#include "RGBAMatrixColorScaleBitmapEffect.h"
#include "RGBAMatrixColorScaleBitmapAnimationEffect.h"
#include "bitmapshapes/ShapeAnimated.h"
#include "bitmapshapes/ShapeDefinitions.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include <algorithm>

namespace DOF
{

RGBAMatrixColorScaleShapeEffect::RGBAMatrixColorScaleShapeEffect()
   : MatrixEffectBase<RGBAColor>()
   , m_activeColor(255, 255, 255, 255)
   , m_inactiveColor(0, 0, 0, 0)
   , m_targetEffect(nullptr)
{
}

RGBAMatrixColorScaleShapeEffect::~RGBAMatrixColorScaleShapeEffect() { }

void RGBAMatrixColorScaleShapeEffect::Init(Table* table)
{
   MatrixEffectBase<RGBAColor>::Init(table);

   Shape* def = nullptr;
   ShapeList& shapes = table->GetShapeDefinitions()->GetShapes();
   for (Shape* sh : shapes)
   {
      if (sh && StringExtensions::EqualsIgnoreCase(sh->GetName(), m_shapeName))
      {
         def = sh;
         break;
      }
   }

   if (def != nullptr)
   {
      IEffect* fx = nullptr;

      ShapeAnimated* defAnim = dynamic_cast<ShapeAnimated*>(def);
      if (defAnim != nullptr)
      {
         RGBAMatrixColorScaleBitmapAnimationEffect* fxa = new RGBAMatrixColorScaleBitmapAnimationEffect();
         fxa->SetAnimationBehaviour(defAnim->GetAnimationBehaviour());
         fxa->SetAnimationFrameCount(defAnim->GetAnimationFrameCount());
         fxa->SetAnimationFrameDurationMs(defAnim->GetAnimationFrameDurationMs());
         fxa->SetAnimationStepDirection(defAnim->GetAnimationStepDirection());
         fxa->SetAnimationStepSize(defAnim->GetAnimationStepSize());
         fxa->SetActiveColor(this->GetActiveColor());
         fxa->SetInactiveColor(this->GetInactiveColor());
         fx = static_cast<EffectBase*>(fxa);
      }
      else
      {
         RGBAMatrixColorScaleBitmapEffect* colorFx = new RGBAMatrixColorScaleBitmapEffect();
         colorFx->SetActiveColor(this->GetActiveColor());
         colorFx->SetInactiveColor(this->GetInactiveColor());
         fx = static_cast<EffectBase*>(colorFx);
      }

      MatrixBitmapEffectBase<RGBAColor>* bitmapFx = dynamic_cast<MatrixBitmapEffectBase<RGBAColor>*>(fx);
      if (bitmapFx)
      {
         bitmapFx->SetBitmapFilePattern(table->GetShapeDefinitions()->GetBitmapFilePattern());
         bitmapFx->SetBitmapFrameNumber(def->GetBitmapFrameNumber());
         bitmapFx->SetBitmapHeight(def->GetBitmapHeight());
         bitmapFx->SetBitmapWidth(def->GetBitmapWidth());
         bitmapFx->SetBitmapTop(def->GetBitmapTop());
         bitmapFx->SetBitmapLeft(def->GetBitmapLeft());
         bitmapFx->SetDataExtractMode(def->GetDataExtractMode());
         bitmapFx->SetToyName(this->GetToyName());
         bitmapFx->SetLayerNr(this->GetLayerNr());
         bitmapFx->SetFadeMode(this->GetFadeMode());
         bitmapFx->SetLeft(this->GetLeft());
         bitmapFx->SetTop(this->GetTop());
         bitmapFx->SetWidth(this->GetWidth());
         bitmapFx->SetHeight(this->GetHeight());
      }

      fx->SetName(this->GetName() + " Target");
      m_targetEffect = fx;

      m_targetEffect->Init(table);
   }
   else
   {
      m_targetEffect = nullptr;
   }
}

void RGBAMatrixColorScaleShapeEffect::Finish()
{
   MatrixEffectBase<RGBAColor>::Finish();

   if (m_targetEffect != nullptr)
   {
      m_targetEffect->Finish();
      delete m_targetEffect;
      m_targetEffect = nullptr;
   }
}

void RGBAMatrixColorScaleShapeEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr)
   {
      TableElementData data = *tableElementData;
      m_targetEffect->Trigger(&data);
   }
}

}