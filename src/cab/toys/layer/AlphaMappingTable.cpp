#include "AlphaMappingTable.h"

namespace DOF
{

float AlphaMappingTable::AlphaMapping[256][256];
bool AlphaMappingTable::s_initialized = false;

void AlphaMappingTable::Initialize()
{
   if (s_initialized)
      return;

   for (int alpha = 0; alpha < 256; alpha++)
   {
      for (int value = 0; value < 256; value++)
         AlphaMapping[alpha][value] = static_cast<float>(static_cast<double>(value) / 255 * static_cast<double>(alpha));
   }

   s_initialized = true;
}

AlphaMappingTableInitializer::AlphaMappingTableInitializer() { AlphaMappingTable::Initialize(); }

}