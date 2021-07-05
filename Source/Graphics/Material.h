#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

enum class BlendMode
{
   Opaque,
   Translucent
};

class Material : public GraphicsResource
{
public:
   Material(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);
   virtual ~Material() = default;

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
