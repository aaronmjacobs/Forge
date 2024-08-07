#pragma once

#include "Graphics/BlendMode.h"
#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceTypes.h"

#include <cstdint>

class DynamicDescriptorPool;
class ResourceManager;

class Material : public GraphicsResource
{
public:
   Material(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager, uint32_t typeFlagBit);

   BlendMode getBlendMode() const
   {
      return blendMode;
   }

   void setBlendMode(BlendMode newBlendMode)
   {
      blendMode = newBlendMode;
   }

   bool isTwoSided() const
   {
      return twoSided;
   }

   void setTwoSided(bool newTwoSided)
   {
      twoSided = newTwoSided;
   }

   uint32_t getTypeFlag() const
   {
      return typeFlag;
   }

   virtual void update()
   {
   }

   friend class MaterialLoader;

protected:
   ResourceManager& resourceManager;
   BlendMode blendMode = BlendMode::Opaque;
   bool twoSided = false;

   MaterialHandle getHandle() const
   {
      return handle;
   }

private:
   MaterialHandle handle;
   uint32_t typeFlag = 0;
};
