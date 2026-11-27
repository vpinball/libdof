#pragma once

#include "MatrixEffectBase.h"
#include "IMatrixBitmapEffect.h"
#include "../../general/bitmap/FastBitmap.h"
#include "../../general/bitmap/FastBitmapDataExtractModeEnum.h"
#include "../../general/bitmap/FastImage.h"
#include "../../general/FilePattern.h"
#include "../../general/FileInfo.h"
#include "../../general/MathExtensions.h"
#include "../../general/StringExtensions.h"
#include "../../table/TableElementData.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../globalconfiguration/GlobalConfig.h"
#include "../../Log.h"

namespace DOF
{

class TableElementData;

template <typename MatrixElementType> class MatrixBitmapEffectBase : public MatrixEffectBase<MatrixElementType>, public virtual IMatrixBitmapEffect
{
public:
   MatrixBitmapEffectBase();
   virtual ~MatrixBitmapEffectBase();

   virtual FilePattern* GetBitmapFilePattern() const override { return m_bitmapFilePattern; }
   virtual void SetBitmapFilePattern(FilePattern* value) override;
   virtual int GetBitmapFrameNumber() const override { return m_bitmapFrameNumber; }
   virtual void SetBitmapFrameNumber(int value) override { m_bitmapFrameNumber = value; }
   virtual int GetBitmapLeft() const override { return m_bitmapLeft; }
   virtual void SetBitmapLeft(int value) override { m_bitmapLeft = value; }
   virtual int GetBitmapTop() const override { return m_bitmapTop; }
   virtual void SetBitmapTop(int value) override { m_bitmapTop = value; }
   virtual int GetBitmapWidth() const override { return m_bitmapWidth; }
   virtual void SetBitmapWidth(int value) override { m_bitmapWidth = value; }
   virtual int GetBitmapHeight() const override { return m_bitmapHeight; }
   virtual void SetBitmapHeight(int value) override { m_bitmapHeight = value; }
   virtual FastBitmapDataExtractModeEnum GetDataExtractMode() const override { return m_dataExtractMode; }
   virtual void SetDataExtractMode(FastBitmapDataExtractModeEnum value) override { m_dataExtractMode = value; }
   virtual void Trigger(TableElementData* tableElementData) override;
   virtual void Init(Table* table) override;
   virtual void Finish() override;

   virtual MatrixElementType GetEffectValue(int triggerValue, PixelData pixel) = 0;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   void OutputBitmap(int fadeValue);
   void CleanupPixels();
   PixelData** m_pixels;

protected:
   bool m_initOK;

private:
   FilePattern* m_bitmapFilePattern;
   FilePattern* m_ownedBitmapFilePattern;

private:
   int m_bitmapFrameNumber;
   int m_bitmapLeft;
   int m_bitmapTop;
   int m_bitmapWidth;
   int m_bitmapHeight;
   FastBitmapDataExtractModeEnum m_dataExtractMode;
};


template <typename MatrixElementType>
MatrixBitmapEffectBase<MatrixElementType>::MatrixBitmapEffectBase()
   : m_initOK(false)
   , m_bitmapFilePattern(nullptr)
   , m_ownedBitmapFilePattern(nullptr)
   , m_pixels(nullptr)
   , m_bitmapFrameNumber(0)
   , m_bitmapLeft(0)
   , m_bitmapTop(0)
   , m_bitmapWidth(-1)
   , m_bitmapHeight(-1)
   , m_dataExtractMode(FastBitmapDataExtractModeEnum::BlendPixels)
{
}

template <typename MatrixElementType> MatrixBitmapEffectBase<MatrixElementType>::~MatrixBitmapEffectBase()
{
   CleanupPixels();
   delete m_ownedBitmapFilePattern;
}

template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::SetBitmapFilePattern(FilePattern* value) { m_bitmapFilePattern = value; }

template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::Trigger(TableElementData* tableElementData)
{
   if (m_initOK)
   {
      OutputBitmap(tableElementData->m_value);
   }
}

template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::Init(Table* table)
{
   m_initOK = false;
   CleanupPixels();
   MatrixEffectBase<MatrixElementType>::Init(table);

   if (m_bitmapFilePattern != nullptr && m_bitmapFilePattern->IsValid())
   {
      FileInfo* fi = m_bitmapFilePattern->GetFirstMatchingFile(table->GetPinball()->GetGlobalConfig()->GetReplaceValuesDictionary());
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
            Log::Exception(StringExtensions::Build("MatrixBitmapEffectBase {0} cant initialize.  Could not load file {1}.", EffectBase::GetName(), fi->FullName()));
            delete fi;
            return;
         }

         const auto& frames = bm->GetFrames();
         auto frameIt = frames.find(m_bitmapFrameNumber);
         if (frameIt != frames.end())
         {
            std::vector<std::string> args = { std::to_string(this->GetAreaWidth()), std::to_string(this->GetAreaHeight()), std::to_string(m_bitmapLeft), std::to_string(m_bitmapTop),
               std::to_string(m_bitmapWidth), std::to_string(m_bitmapHeight) };
            Log::Instrumentation("MX", StringExtensions::Build("BitmapEffectBase. Grabbing image clip: W: {0}, H:{1}, BML: {2}, BMT: {3}, BMW: {4}, BMH: {5}", args));
            FastBitmap clippedBitmap = frameIt->second.GetClip(this->GetAreaWidth(), this->GetAreaHeight(), m_bitmapLeft, m_bitmapTop, m_bitmapWidth, m_bitmapHeight, m_dataExtractMode);
            m_pixels = clippedBitmap.GetPixels();
         }
         else
         {
            Log::Warning(StringExtensions::Build(
               "MatrixBitmapEffectBase {0} cant initialize. Frame {1} does not exist in source image {2}.", EffectBase::GetName(), std::to_string(m_bitmapFrameNumber), fi->FullName()));
         }
         delete fi;
      }
      else
      {
         Log::Warning(StringExtensions::Build(
            "MatrixBitmapEffectBase {0} cant initialize. No file matches the BitmapFilePattern {1} is invalid", EffectBase::GetName(), m_bitmapFilePattern->GetPattern()));
         delete fi;
      }
   }
   else
   {
      Log::Warning(StringExtensions::Build(
         "MatrixBitmapEffectBase {0} cant initialize. The BitmapFilePattern {1} is invalid", EffectBase::GetName(), m_bitmapFilePattern ? m_bitmapFilePattern->GetPattern() : "(null)"));
   }

   m_initOK = (m_pixels != nullptr && this->m_matrixLayer != nullptr);
}

template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::CleanupPixels()
{
   if (m_pixels != nullptr)
   {
      for (int x = 0; x < this->GetAreaWidth(); x++)
      {
         delete[] m_pixels[x];
      }
      delete[] m_pixels;
      m_pixels = nullptr;
   }
}

template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::Finish()
{
   CleanupPixels();
   MatrixEffectBase<MatrixElementType>::Finish();
}


template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::OutputBitmap(int fadeValue)
{
   if (this->GetFadeMode() == FadeModeEnum::OnOff)
      fadeValue = (fadeValue < 1 ? 0 : 255);

   for (int y = 0; y < this->GetAreaHeight(); y++)
   {
      int yd = y + this->m_areaTop;
      for (int x = 0; x < this->GetAreaWidth(); x++)
      {
         int xd = x + this->m_areaLeft;
         this->m_matrix->SetElement(this->GetLayerNr(), xd, yd, GetEffectValue(fadeValue, m_pixels[x][y]));
      }
   }
}

template <typename MatrixElementType> void MatrixBitmapEffectBase<MatrixElementType>::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixEffectBase<MatrixElementType>::DeserializeFromXml(element);

   std::string text;
   int value;
   if (EffectBase::ReadXmlInt(element, "BitmapFrameNumber", value))
      this->SetBitmapFrameNumber(value);
   if (EffectBase::ReadXmlInt(element, "BitmapTop", value))
      this->SetBitmapTop(value);
   if (EffectBase::ReadXmlInt(element, "BitmapLeft", value))
      this->SetBitmapLeft(value);
   if (EffectBase::ReadXmlInt(element, "BitmapWidth", value))
      this->SetBitmapWidth(value);
   if (EffectBase::ReadXmlInt(element, "BitmapHeight", value))
      this->SetBitmapHeight(value);
   if (EffectBase::ReadXmlString(element, "DataExtractMode", text))
   {
      if (text == "SinglePixelTopLeft")
         this->SetDataExtractMode(FastBitmapDataExtractModeEnum::SinglePixelTopLeft);
      else if (text == "SinglePixelCenter")
         this->SetDataExtractMode(FastBitmapDataExtractModeEnum::SinglePixelCenter);
      else if (text == "BlendPixels")
         this->SetDataExtractMode(FastBitmapDataExtractModeEnum::BlendPixels);
      else
         EffectBase::ThrowInvalidXmlValue("DataExtractMode", text);
   }
   if (EffectBase::ReadXmlString(element, "BitmapFilePattern", text))
   {
      delete m_ownedBitmapFilePattern;
      m_ownedBitmapFilePattern = new FilePattern(text);
      this->SetBitmapFilePattern(m_ownedBitmapFilePattern);
   }
}

}