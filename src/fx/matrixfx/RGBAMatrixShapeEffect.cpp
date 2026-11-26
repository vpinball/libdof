#include "RGBAMatrixShapeEffect.h"
#include "RGBAMatrixBitmapEffect.h"
#include "RGBAMatrixBitmapAnimationEffect.h"
#include "IMatrixBitmapEffect.h"
#include "IMatrixEffect.h"
#include "bitmapshapes/ShapeAnimated.h"
#include "bitmapshapes/ShapeDefinitions.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include <algorithm>

namespace DOF
{

RGBAMatrixShapeEffect::RGBAMatrixShapeEffect()
   : MatrixEffectBase<RGBAColor>()
   , m_targetEffect(nullptr)
{
}

RGBAMatrixShapeEffect::~RGBAMatrixShapeEffect() { }

const std::string& RGBAMatrixShapeEffect::GetShapeName() const { return m_shapeName; }

void RGBAMatrixShapeEffect::SetShapeName(const std::string& value) { m_shapeName = value; }

void RGBAMatrixShapeEffect::Init(Table* table)
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
      IMatrixBitmapEffect* fx;
      ShapeAnimated* defAnim = dynamic_cast<ShapeAnimated*>(def);
      if (defAnim != nullptr)
      {
         fx = new RGBAMatrixBitmapAnimationEffect();

         RGBAMatrixBitmapAnimationEffect* fxa = dynamic_cast<RGBAMatrixBitmapAnimationEffect*>(fx);

         fxa->SetAnimationBehaviour(defAnim->GetAnimationBehaviour());
         fxa->SetAnimationFrameCount(defAnim->GetAnimationFrameCount());
         fxa->SetAnimationFrameDurationMs(defAnim->GetAnimationFrameDurationMs());
         fxa->SetAnimationStepDirection(defAnim->GetAnimationStepDirection());
         fxa->SetAnimationStepSize(defAnim->GetAnimationStepSize());
      }
      else
      {
         fx = new RGBAMatrixBitmapEffect();
      }

      fx->SetBitmapFilePattern(table->GetShapeDefinitions()->GetBitmapFilePattern());
      fx->SetBitmapFrameNumber(def->GetBitmapFrameNumber());
      fx->SetBitmapHeight(def->GetBitmapHeight());
      fx->SetBitmapWidth(def->GetBitmapWidth());
      fx->SetBitmapTop(def->GetBitmapTop());
      fx->SetBitmapLeft(def->GetBitmapLeft());
      fx->SetDataExtractMode(def->GetDataExtractMode());
      fx->SetToyName(this->GetToyName());
      fx->SetLayerNr(this->GetLayerNr());
      fx->SetFadeMode(this->GetFadeMode());
      fx->SetLeft(this->GetLeft());
      fx->SetTop(this->GetTop());
      fx->SetWidth(this->GetWidth());
      fx->SetHeight(this->GetHeight());

      fx->SetName(this->GetName() + " Target");

      m_targetEffect = fx;
      m_targetEffect->Init(table);
   }
   else
   {
      m_targetEffect = nullptr;
   }
}

void RGBAMatrixShapeEffect::Finish()
{
   MatrixEffectBase<RGBAColor>::Finish();

   if (m_targetEffect != nullptr)
   {
      m_targetEffect->Finish();
      delete m_targetEffect;
      m_targetEffect = nullptr;
   }
}

void RGBAMatrixShapeEffect::Trigger(TableElementData* tableElementData)
{
   if (m_targetEffect != nullptr)
   {
      TableElementData data = *tableElementData;
      m_targetEffect->Trigger(&data);
   }
}

}