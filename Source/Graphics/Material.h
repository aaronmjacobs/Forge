#pragma once

#include "Graphics/BlendMode.h"
#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceTypes.h"

class DynamicDescriptorPool;
class MaterialLoader;

class Material : public GraphicsResource
{
public:
   Material(const GraphicsContext& graphicsContext, MaterialLoader& owningMaterialLoader, const vk::DescriptorSetLayoutCreateInfo& createInfo);

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

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

   virtual void update()
   {
   }

   friend class MaterialLoader;

protected:
   MaterialLoader& materialLoader;
   DescriptorSet descriptorSet;
   BlendMode blendMode = BlendMode::Opaque;
   bool twoSided = false;

   MaterialHandle getHandle() const
   {
      return handle;
   }

private:
   MaterialHandle handle;
};
