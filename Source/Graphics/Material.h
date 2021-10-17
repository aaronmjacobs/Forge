#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

class DynamicDescriptorPool;

enum class BlendMode
{
   Opaque,
   Translucent
};

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

protected:
   DescriptorSet descriptorSet;
   BlendMode blendMode = BlendMode::Opaque;
};
