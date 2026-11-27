#pragma once

#include "../EffectBase.h"
#include "IMatrixEffect.h"
#include "../../cab/toys/IMatrixToy.h"
#include "../../cab/toys/ToyList.h"
#include "../../cab/toys/IToy.h"
#include "../../table/Table.h"
#include "../../Pinball.h"
#include "../../cab/Cabinet.h"
#include "../../general/MathExtensions.h"
#include "../../general/StringExtensions.h"
#include "../../Log.h"
#include <cmath>

namespace DOF
{

class Table;
class TableElementData;

template <typename MatrixElementType> class MatrixEffectBase : public EffectBase, public virtual IMatrixEffect
{
public:
   MatrixEffectBase();
   virtual ~MatrixEffectBase() = default;

   virtual FadeModeEnum GetFadeMode() const override { return m_fadeMode; }
   virtual void SetFadeMode(FadeModeEnum value) override { m_fadeMode = value; }
   virtual float GetHeight() const override { return m_height; }
   virtual void SetHeight(float value) override { m_height = MathExtensions::Limit(value, 0.0f, 100.0f); }
   virtual int GetLayerNr() const override { return m_layerNr; }
   virtual void SetLayerNr(int value) override { m_layerNr = value; }
   virtual float GetLeft() const override { return m_left; }
   virtual void SetLeft(float value) override { m_left = MathExtensions::Limit(value, 0.0f, 100.0f); }
   virtual float GetTop() const override { return m_top; }
   virtual void SetTop(float value) override { m_top = MathExtensions::Limit(value, 0.0f, 100.0f); }
   virtual const std::string& GetToyName() const override { return m_toyName; }
   virtual void SetToyName(const std::string& value) override;
   virtual float GetWidth() const override { return m_width; }
   virtual void SetWidth(float value) override { m_width = MathExtensions::Limit(value, 0.0f, 100.0f); }
   virtual void Init(Table* table) override;
   virtual void Finish() override;

protected:
   virtual void DeserializeFromXml(const tinyxml2::XMLElement* element) override;

   IMatrixToy<MatrixElementType>* m_matrix;
   MatrixElementType* m_matrixLayer;
   Table* m_table;
   int m_areaLeft;
   int m_areaTop;
   int m_areaRight;
   int m_areaBottom;

   int GetAreaWidth() const { return (m_areaRight - m_areaLeft) + 1; }
   int GetAreaHeight() const { return (m_areaBottom - m_areaTop) + 1; }

private:
   std::string m_toyName;
   float m_width;
   float m_height;
   float m_left;
   float m_top;
   int m_layerNr;
   FadeModeEnum m_fadeMode;
};

template <typename MatrixElementType>
MatrixEffectBase<MatrixElementType>::MatrixEffectBase()
   : m_matrix(nullptr)
   , m_matrixLayer(nullptr)
   , m_table(nullptr)
   , m_areaLeft(0)
   , m_areaTop(0)
   , m_areaRight(0)
   , m_areaBottom(0)
   , m_width(100.0f)
   , m_height(100.0f)
   , m_left(0.0f)
   , m_top(0.0f)
   , m_layerNr(0)
   , m_fadeMode(FadeModeEnum::Fade)
{
}

template <typename MatrixElementType> void MatrixEffectBase<MatrixElementType>::SetToyName(const std::string& value)
{
   if (m_toyName != value)
   {
      m_toyName = value;
      m_matrix = nullptr;
      m_matrixLayer = nullptr;
   }
}

template <typename MatrixElementType> void MatrixEffectBase<MatrixElementType>::Init(Table* table)
{
   m_table = table;
   if (!StringExtensions::IsNullOrWhiteSpace(m_toyName) && table->GetPinball()->GetCabinet()->GetToys()->Contains(m_toyName))
   {
      IToy* toy = table->GetPinball()->GetCabinet()->GetToys()->FindByName(m_toyName);
      IMatrixToy<MatrixElementType>* matrixToy = dynamic_cast<IMatrixToy<MatrixElementType>*>(toy);
      if (matrixToy != nullptr)
      {
         m_matrix = matrixToy;
         m_matrixLayer = m_matrix->GetLayer(m_layerNr);

         m_areaLeft = MathExtensions::Limit((int)std::floor((float)m_matrix->GetWidth() / 100.0f * m_left), 0, m_matrix->GetWidth() - 1);
         m_areaTop = MathExtensions::Limit((int)std::floor((float)m_matrix->GetHeight() / 100.0f * m_top), 0, m_matrix->GetHeight() - 1);
         m_areaRight = MathExtensions::Limit((int)std::floor((float)m_matrix->GetWidth() / 100.0f * MathExtensions::Limit(m_left + m_width, 0.0f, 100.0f)), 0, m_matrix->GetWidth() - 1);
         m_areaBottom = MathExtensions::Limit((int)std::floor((float)m_matrix->GetHeight() / 100.0f * MathExtensions::Limit(m_top + m_height, 0.0f, 100.0f)), 0, m_matrix->GetHeight() - 1);

         int tmp;
         if (m_areaLeft > m_areaRight)
         {
            tmp = m_areaRight;
            m_areaRight = m_areaLeft;
            m_areaLeft = m_areaRight;
         }
         if (m_areaTop > m_areaBottom)
         {
            tmp = m_areaBottom;
            m_areaBottom = m_areaTop;
            m_areaTop = tmp;
         }

         std::vector<std::string> args = { std::to_string(m_left), std::to_string(m_top), std::to_string(m_width), std::to_string(m_height), std::to_string(m_matrix->GetHeight()),
            std::to_string(m_matrix->GetWidth()) };
         std::vector<std::string> args2 = { std::to_string(m_areaLeft), std::to_string(m_areaTop), std::to_string(m_areaRight), std::to_string(m_areaBottom), std::to_string(GetAreaWidth()),
            std::to_string(GetAreaHeight()) };
         args.insert(args.end(), args2.begin(), args2.end());
         args.push_back(this->GetXmlElementName());
         Log::Instrumentation("MX",
            StringExtensions::Build("MatrixBase for {12}. Calculated area size: AreaDef(L:{0}, T:{1}, W:{2}, H:{3}), Matrix(W:{4}, H:{5}), ResultArea(Left: {6}, Top:{7}, Right:{8}, "
                                    "Bottom:{9}, Width:{10}, Height:{11})",
               args));
      }
   }
}

template <typename MatrixElementType> void MatrixEffectBase<MatrixElementType>::Finish()
{
   m_matrixLayer = nullptr;
   m_matrix = nullptr;
   m_table = nullptr;
   EffectBase::Finish();
}

template <typename MatrixElementType> void MatrixEffectBase<MatrixElementType>::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   EffectBase::DeserializeFromXml(element);

   std::string text;
   int value;
   float floatValue;
   FadeModeEnum fadeMode;
   if (EffectBase::ReadXmlString(element, "ToyName", text))
      this->SetToyName(text);
   if (EffectBase::ReadXmlFloat(element, "Width", floatValue))
      this->SetWidth(floatValue);
   if (EffectBase::ReadXmlFloat(element, "Height", floatValue))
      this->SetHeight(floatValue);
   if (EffectBase::ReadXmlFloat(element, "Left", floatValue))
      this->SetLeft(floatValue);
   if (EffectBase::ReadXmlFloat(element, "Top", floatValue))
      this->SetTop(floatValue);
   if (EffectBase::ReadXmlInt(element, "LayerNr", value))
      this->SetLayerNr(value);
   if (EffectBase::ReadXmlFadeMode(element, "FadeMode", fadeMode))
      this->SetFadeMode(fadeMode);
}

}