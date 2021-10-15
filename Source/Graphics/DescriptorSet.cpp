#include "Graphics/DescriptorSet.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"

#include <array>

DescriptorSet::DescriptorSet(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
{
   vk::DescriptorSetLayout layout = graphicsContext.getLayoutCache().getLayout(createInfo);

   std::array<vk::DescriptorSetLayout, GraphicsContext::kMaxFramesInFlight> layouts;
   layouts.fill(layout);

   vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(descriptorPool)
      .setSetLayouts(layouts);

   vk::Result result = device.allocateDescriptorSets(&allocateInfo, sets.data());
   ASSERT(result == vk::Result::eSuccess);

   for (std::size_t i = 0; i < sets.size(); ++i)
   {
      NAME_CHILD(sets[i], "Descriptor Set " + std::to_string(i));
   }
}
