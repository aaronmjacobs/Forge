#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

class Material : public GraphicsResource
{
public:
   Material(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);
   virtual ~Material() = default;

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

protected:
   DescriptorSet descriptorSet;
};
