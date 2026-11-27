#include "RGBAMatrixColorScaleBitmapEffect.h"
#include "../../general/MathExtensions.h"

namespace DOF
{

RGBAMatrixColorScaleBitmapEffect::RGBAMatrixColorScaleBitmapEffect()
   : MatrixBitmapEffectBase<RGBAColor>()
   , m_activeColor(255, 255, 255, 255)
   , m_inactiveColor(0, 0, 0, 0)
{
}

void RGBAMatrixColorScaleBitmapEffect::Init(Table* table)
{
   MatrixBitmapEffectBase<RGBAColor>::Init(table);

   if (this->m_pixels != nullptr)
   {
      for (int y = 0; y < this->GetAreaHeight(); y++)
      {
         for (int x = 0; x < this->GetAreaWidth(); x++)
         {
            PixelData p = this->m_pixels[x][y];

            double brightness = ((double)(p.red + p.green + p.blue) / 3.0);
            brightness = MathExtensions::Limit((int)brightness, 0, 255);

            p.red = MathExtensions::Limit((int)(m_inactiveColor.GetRed() + ((float)(m_activeColor.GetRed() - m_inactiveColor.GetRed()) * brightness / 255.0f)), 0, 255);
            p.green = MathExtensions::Limit((int)(m_inactiveColor.GetGreen() + ((float)(m_activeColor.GetGreen() - m_inactiveColor.GetGreen()) * brightness / 255.0f)), 0, 255);
            p.blue = MathExtensions::Limit((int)(m_inactiveColor.GetBlue() + ((float)(m_activeColor.GetBlue() - m_inactiveColor.GetBlue()) * brightness / 255.0f)), 0, 255);
            p.alpha = MathExtensions::Limit((int)(m_inactiveColor.GetAlpha() + ((float)(m_activeColor.GetAlpha() - m_inactiveColor.GetAlpha()) * brightness / 255.0f)), 0, 255);

            this->m_pixels[x][y] = p;
         }
      }
   }
}

RGBAColor RGBAMatrixColorScaleBitmapEffect::GetEffectValue(int triggerValue, PixelData pixel)
{
   RGBAColor d = pixel.GetRGBAColor();
   d.SetAlpha((int)((float)pixel.alpha * triggerValue / 255));
   return d;
}

void RGBAMatrixColorScaleBitmapEffect::DeserializeFromXml(const tinyxml2::XMLElement* element)
{
   MatrixBitmapEffectBase<RGBAColor>::DeserializeFromXml(element);

   RGBAColor color;
   if (ReadXmlRGBAColor(element, "ActiveColor", color))
      SetActiveColor(color);
   if (ReadXmlRGBAColor(element, "InactiveColor", color))
      SetInactiveColor(color);
}

}