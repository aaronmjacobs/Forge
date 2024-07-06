#include "Graphics/Material.h"

namespace
{
   bool isPowerOfTwo(uint32_t flag)
   {
      return (flag & (flag - 1)) == 0;
   }
}

Material::Material(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager, uint32_t typeFlagBit)
   : GraphicsResource(graphicsContext)
   , resourceManager(owningResourceManager)
   , typeFlag(typeFlagBit)
{
   ASSERT(isPowerOfTwo(typeFlag));
}
