#pragma once

#include "MatrixBitmapEffectBase.h"
#include "MatrixAnimationStepDirectionEnum.h"
#include "AnimationBehaviourEnum.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include "../../pinballsupport/AlarmHandler.h"
#include "../../pinballsupport/Action.h"
#include "../../general/bitmap/PixelData.h"
#include <chrono>
#include <vector>

namespace DOF
{

class TableElementData;

template <typename MatrixElementType> class MatrixBitmapAnimationEffectBase : public MatrixBitmapEffectBase<MatrixElementType>
{
public:
   MatrixBitmapAnimationEffectBase();
   virtual ~MatrixBitmapAnimationEffectBase();

   int GetAnimationFrameCount() const { return m_animationFrameCount; }
   void SetAnimationFrameCount(int value) { m_animationFrameCount = MathExtensions::Limit(value, 1, INT_MAX); }
   int GetAnimationStepSize() const { return m_animationStepSize; }
   void SetAnimationStepSize(int value) { m_animationStepSize = value; }
   int GetAnimationFrameDurationMs() const { return m_animationFrameDurationMs; }
   void SetAnimationFrameDurationMs(int value) { m_animationFrameDurationMs = MathExtensions::Limit(value, 1, INT_MAX); }
   MatrixAnimationStepDirectionEnum GetAnimationStepDirection() const { return m_animationStepDirection; }
   void SetAnimationStepDirection(MatrixAnimationStepDirectionEnum value) { m_animationStepDirection = value; }
   AnimationBehaviourEnum GetAnimationBehaviour() const { return m_animationBehaviour; }
   void SetAnimationBehaviour(AnimationBehaviourEnum value) { m_animationBehaviour = value; }
   virtual void SetBitmapTop(int value) override { MatrixBitmapEffectBase<MatrixElementType>::SetBitmapTop(MathExtensions::Limit(value, 0, INT_MAX)); }
   virtual void SetBitmapLeft(int value) override { MatrixBitmapEffectBase<MatrixElementType>::SetBitmapLeft(MathExtensions::Limit(value, 0, INT_MAX)); }
   virtual void SetBitmapWidth(int value) override { MatrixBitmapEffectBase<MatrixElementType>::SetBitmapWidth(MathExtensions::Limit(value, -1, INT_MAX)); }
   virtual void SetBitmapHeight(int value) override { MatrixBitmapEffectBase<MatrixElementType>::SetBitmapHeight(MathExtensions::Limit(value, -1, INT_MAX)); }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   void ControlAnimation(int fadeValue);
   void Animate();
   void StopAnimation();
   void CleanupPixels();

   bool m_animationActive;
   int m_animationStep;
   int m_animationFadeValue;
   std::vector<PixelData**> m_pixels;

private:
   int m_animationFrameCount;
   int m_animationStepSize;
   int m_animationFrameDurationMs;
   MatrixAnimationStepDirectionEnum m_animationStepDirection;
   AnimationBehaviourEnum m_animationBehaviour;
};


template <typename MatrixElementType>
MatrixBitmapAnimationEffectBase<MatrixElementType>::MatrixBitmapAnimationEffectBase()
   : m_animationActive(false)
   , m_animationStep(0)
   , m_animationFadeValue(0)
   , m_animationFrameCount(1)
   , m_animationStepSize(1)
   , m_animationFrameDurationMs(30)
   , m_animationStepDirection(MatrixAnimationStepDirectionEnum::Frame)
   , m_animationBehaviour(AnimationBehaviourEnum::Loop)
{
}

template <typename MatrixElementType> MatrixBitmapAnimationEffectBase<MatrixElementType>::~MatrixBitmapAnimationEffectBase() { CleanupPixels(); }

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::Trigger(TableElementData* tableElementData)
{
   if (this->m_initOK)
   {
      int fadeValue = tableElementData->m_value;
      if (this->GetFadeMode() == FadeModeEnum::OnOff)
         fadeValue = (fadeValue < 1 ? 0 : 255);
      ControlAnimation(fadeValue);
   }
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::ControlAnimation(int fadeValue)
{
   if (fadeValue > 0)
   {
      m_animationFadeValue = fadeValue;

      if (!m_animationActive)
      {
         m_animationActive = true;

         if (m_animationBehaviour != AnimationBehaviourEnum::Continue)
         {
            m_animationStep = 0;
         }
         this->m_table->GetPinball()->GetAlarms()->RegisterIntervalAlarm(m_animationFrameDurationMs, Action(this, &MatrixBitmapAnimationEffectBase<MatrixElementType>::Animate));

         Animate();
      }
   }
   else
   {
      StopAnimation();
   }
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::StopAnimation()
{
   if (m_animationActive)
   {
      try
      {
         this->m_table->GetPinball()->GetAlarms()->UnregisterIntervalAlarm(Action(this, &MatrixBitmapAnimationEffectBase<MatrixElementType>::Animate));
      }
      catch (...)
      {
      }

      m_animationActive = false;

      for (int y = this->m_areaTop; y <= this->m_areaBottom; y++)
      {
         for (int x = this->m_areaLeft; x <= this->m_areaRight; x++)
         {
            this->m_matrix->SetElement(this->GetLayerNr(), x, y, this->GetEffectValue(0, PixelData()));
         }
      }
   }
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::Animate()
{
   if (m_animationStep < static_cast<int>(m_pixels.size()))
   {
      for (int y = 0; y < this->GetAreaHeight(); y++)
      {
         int yd = y + this->m_areaTop;
         for (int x = 0; x < this->GetAreaWidth(); x++)
         {
            int xd = x + this->m_areaLeft;
            this->m_matrix->SetElement(this->GetLayerNr(), xd, yd, this->GetEffectValue(m_animationFadeValue, m_pixels[m_animationStep][x][y]));
         }
      }
      m_animationStep++;
      if (m_animationBehaviour != AnimationBehaviourEnum::Once)
      {
         m_animationStep = m_animationStep % static_cast<int>(m_pixels.size());
      }
   }
   else
   {
      m_animationStep = 0;
      if (m_animationBehaviour == AnimationBehaviourEnum::Once)
      {
         StopAnimation();
      }
   }
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::Init(Table* table)
{
   this->m_initOK = false;
   CleanupPixels();
   MatrixEffectBase<MatrixElementType>::Init(table);

   if (this->GetBitmapFilePattern() != nullptr && this->GetBitmapFilePattern()->IsValid())
   {
      FileInfo* fi = this->GetBitmapFilePattern()->GetFirstMatchingFile(table->GetPinball()->GetGlobalConfig()->GetReplaceValuesDictionary());
      if (fi != nullptr && fi->Exists())
      {
         FastImage* bm = nullptr;
         try
         {
            auto& bitmaps = table->GetBitmaps();
            bm = &bitmaps[fi->FullName()];
         }
         catch (...)
         {
            Log::Exception(StringExtensions::Build("MatrixBitmapAnimationEffectBase {0} cant initialize.  Could not load file {1}.", this->GetName(), fi->FullName()));
            delete fi;
            return;
         }

         const auto& frames = bm->GetFrames();
         auto frameIt = frames.find(this->GetBitmapFrameNumber());
         if (frameIt != frames.end())
         {
            int stepCount = m_animationFrameCount;
            switch (m_animationStepDirection)
            {
            case MatrixAnimationStepDirectionEnum::Frame:
            {
               if ((this->GetBitmapFrameNumber() + (stepCount * m_animationStepSize)) > static_cast<int>(frames.size()))
               {
                  stepCount = (static_cast<int>(frames.size()) - this->GetBitmapFrameNumber()) / m_animationStepSize;
               }

               m_pixels.resize(stepCount);
               for (int s = 0; s < stepCount; s++)
               {
                  auto stepFrameIt = frames.find(this->GetBitmapFrameNumber() + s);
                  if (stepFrameIt != frames.end())
                  {
                     FastBitmap clippedBitmap = stepFrameIt->second.GetClip(this->GetAreaWidth(), this->GetAreaHeight(), this->GetBitmapLeft(), this->GetBitmapTop(), this->GetBitmapWidth(),
                        this->GetBitmapHeight(), this->GetDataExtractMode());
                     m_pixels[s] = clippedBitmap.GetPixels();
                  }
               }
               break;
            }
            case MatrixAnimationStepDirectionEnum::Right:
            {
               m_pixels.resize(stepCount);
               for (int s = 0; s < stepCount; s++)
               {
                  FastBitmap clippedBitmap = frameIt->second.GetClip(this->GetAreaWidth(), this->GetAreaHeight(), this->GetBitmapLeft() + s * m_animationStepSize, this->GetBitmapTop(),
                     this->GetBitmapWidth(), this->GetBitmapHeight(), this->GetDataExtractMode());
                  m_pixels[s] = clippedBitmap.GetPixels();
               }
               break;
            }
            case MatrixAnimationStepDirectionEnum::Down:
            {
               m_pixels.resize(stepCount);
               for (int s = 0; s < stepCount; s++)
               {
                  FastBitmap clippedBitmap = frameIt->second.GetClip(this->GetAreaWidth(), this->GetAreaHeight(), this->GetBitmapLeft(), this->GetBitmapTop() + s * m_animationStepSize,
                     this->GetBitmapWidth(), this->GetBitmapHeight(), this->GetDataExtractMode());
                  m_pixels[s] = clippedBitmap.GetPixels();
               }
               break;
            }
            default:
            {
               stepCount = 1;
               m_pixels.resize(stepCount);
               FastBitmap clippedBitmap = frameIt->second.GetClip(
                  this->GetAreaWidth(), this->GetAreaHeight(), this->GetBitmapLeft(), this->GetBitmapTop(), this->GetBitmapWidth(), this->GetBitmapHeight(), this->GetDataExtractMode());
               m_pixels[0] = clippedBitmap.GetPixels();
               break;
            }
            }

            std::vector<std::string> args = { std::to_string(this->GetAreaWidth()), std::to_string(this->GetAreaHeight()), std::to_string(this->GetBitmapLeft()),
               std::to_string(this->GetBitmapTop()), std::to_string(this->GetBitmapWidth()), std::to_string(this->GetBitmapHeight()), std::to_string(stepCount) };
            Log::Instrumentation("MX", StringExtensions::Build("BitmapAnimationEffectBase. Grabbed image clips: W: {0}, H:{1}, BML: {2}, BMT: {3}, BMW: {4}, BMH: {5}, Steps: {6}", args));
         }
         else
         {
            Log::Warning(StringExtensions::Build("MatrixBitmapAnimationEffectBase {0} cant initialize. Frame {1} does not exist in source image {2}.", this->GetName(),
               std::to_string(this->GetBitmapFrameNumber()), fi->FullName()));
         }
         delete fi;
      }
      else
      {
         Log::Warning(StringExtensions::Build(
            "MatrixBitmapAnimationEffectBase {0} cant initialize. No file matches the BitmapFilePattern {1} is invalid", this->GetName(), this->GetBitmapFilePattern()->GetPattern()));
         delete fi;
      }
   }
   else
   {
      Log::Warning(StringExtensions::Build("MatrixBitmapAnimationEffectBase {0} cant initialize. The BitmapFilePattern {1} is invalid", this->GetName(),
         this->GetBitmapFilePattern() ? this->GetBitmapFilePattern()->GetPattern() : "(null)"));
   }

   this->m_initOK = (!m_pixels.empty() && this->m_matrixLayer != nullptr);
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::CleanupPixels()
{
   for (auto pixels : m_pixels)
   {
      if (pixels != nullptr)
      {
         for (int x = 0; x < this->GetAreaWidth(); x++)
         {
            delete[] pixels[x];
         }
         delete[] pixels;
      }
   }
   m_pixels.clear();
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::Finish()
{
   StopAnimation();
   CleanupPixels();
   MatrixEffectBase<MatrixElementType>::Finish();
}

template <typename MatrixElementType> void MatrixBitmapAnimationEffectBase<MatrixElementType>::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixBitmapEffectBase<MatrixElementType>::DeserializeFromXml(element);

   std::string text;
   int value;
   if (EffectBase::ReadXmlString(element, "AnimationStepDirection", text))
   {
      if (text == "Frame")
         this->SetAnimationStepDirection(MatrixAnimationStepDirectionEnum::Frame);
      else if (text == "Right")
         this->SetAnimationStepDirection(MatrixAnimationStepDirectionEnum::Right);
      else if (text == "Down")
         this->SetAnimationStepDirection(MatrixAnimationStepDirectionEnum::Down);
      else
         EffectBase::ThrowInvalidXmlValue("AnimationStepDirection", text);
   }
   if (EffectBase::ReadXmlInt(element, "AnimationStepSize", value))
      this->SetAnimationStepSize(value);
   if (EffectBase::ReadXmlInt(element, "AnimationFrameCount", value))
      this->SetAnimationFrameCount(value);
   if (EffectBase::ReadXmlString(element, "AnimationBehaviour", text))
   {
      if (text == "Once")
         this->SetAnimationBehaviour(AnimationBehaviourEnum::Once);
      else if (text == "Loop")
         this->SetAnimationBehaviour(AnimationBehaviourEnum::Loop);
      else if (text == "Continue")
         this->SetAnimationBehaviour(AnimationBehaviourEnum::Continue);
      else
         EffectBase::ThrowInvalidXmlValue("AnimationBehaviour", text);
   }
   if (EffectBase::ReadXmlInt(element, "AnimationFrameDurationMs", value))
      this->SetAnimationFrameDurationMs(value);
}

}
