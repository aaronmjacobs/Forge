#pragma once

#include "Graphics/BlendMode.h"
#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

class DynamicDescriptorPool;

class Material : public GraphicsResource
{
public:
   Material(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

   BlendMode getBlendMode() const
   {
      return blendMode;
   }

   bool isTwoSided() const
   {
      return twoSided;
   }

protected:
   DescriptorSet descriptorSet;
   BlendMode blendMode = BlendMode::Opaque;
   bool twoSided = false;
};
