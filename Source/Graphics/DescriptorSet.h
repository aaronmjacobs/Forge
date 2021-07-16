#pragma once

#include "Graphics/GraphicsResource.h"

#include <vector>

class DescriptorSet : public GraphicsResource
{
public:
   DescriptorSet(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);

   vk::DescriptorSet getSet(uint32_t frameIndex) const
   {
      return sets[frameIndex];
   }

   vk::DescriptorSet getCurrentSet() const
   {
      return getSet(context.getFrameIndex());
   }

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

private:
   std::vector<vk::DescriptorSet> sets;
};
