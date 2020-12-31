#pragma once

#include "Graphics/GraphicsResource.h"

#include <vector>

class DescriptorSet : public GraphicsResource
{
public:
   DescriptorSet(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);

   vk::DescriptorSetLayout getLayout() const
   {
      return layout;
   }

   vk::DescriptorSet getSet(uint32_t frameIndex) const
   {
      return sets[frameIndex];
   }

   vk::DescriptorSet getCurrentSet() const
   {
      return getSet(context.getFrameIndex());
   }

private:
   vk::DescriptorPool pool;
   vk::DescriptorSetLayout layout;
   std::vector<vk::DescriptorSet> sets;
};
