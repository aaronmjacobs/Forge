#include "Graphics/Material.h"

namespace
{
   bool isPowerOfTwo(uint32_t flag)
   {
      return (flag & (flag - 1)) == 0;
   }
}

Material::Material(const GraphicsContext& graphicsContext, MaterialLoader& owningMaterialLoader, uint32_t typeFlagBit)
   : GraphicsResource(graphicsContext)
   , materialLoader(owningMaterialLoader)
   , typeFlag(typeFlagBit)
{
   ASSERT(isPowerOfTwo(typeFlag));
}
