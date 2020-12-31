#include "Graphics/DescriptorSet.h"

#include "Graphics/DescriptorSetLayoutCache.h"

DescriptorSet::DescriptorSet(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
   , pool(descriptorPool)
   , layout(graphicsContext.getLayoutCache().getLayout(createInfo))
{

   std::vector<vk::DescriptorSetLayout> layouts(GraphicsContext::kMaxFramesInFlight, layout);
   vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(pool)
      .setSetLayouts(layouts);

   sets = device.allocateDescriptorSets(allocateInfo);
}
