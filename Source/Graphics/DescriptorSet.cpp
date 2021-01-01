#include "Graphics/DescriptorSet.h"

#include "Graphics/DescriptorSetLayoutCache.h"

#include <array>

DescriptorSet::DescriptorSet(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , pool(descriptorPool)
   , layout(graphicsContext.getLayoutCache().getLayout(createInfo))
{

   std::array<vk::DescriptorSetLayout, GraphicsContext::kMaxFramesInFlight> layouts;
   layouts.fill(layout);

   vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(pool)
      .setSetLayouts(layouts);

   sets = device.allocateDescriptorSets(allocateInfo);
}
