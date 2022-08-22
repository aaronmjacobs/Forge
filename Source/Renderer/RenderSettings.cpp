#include "Renderer/RenderSettings.h"

#include "Core/Assert.h"

// static
const char* RenderSettings::getQualityString(RenderQuality quality)
{
   switch (quality)
   {
   case RenderQuality::Disabled:
      return "Disabled";
   case RenderQuality::Low:
      return "Low";
   case RenderQuality::Medium:
      return "Medium";
   case RenderQuality::High:
      return "High";
   default:
      ASSERT(false);
      return nullptr;
   }
}
