#include "Graphics/DescriptorSet.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DynamicDescriptorPool.h"

#include <array>

DescriptorSet::DescriptorSet(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo)
   : GraphicsResource(graphicsContext)
{
   vk::DescriptorSetLayout layout = context.getDescriptorSetLayout(createInfo);

   std::array<vk::DescriptorSetLayout, GraphicsContext::kMaxFramesInFlight> layouts;
   layouts.fill(layout);

   vk::DescriptorPool descriptorPool = dynamicDescriptorPool.obtainPool(createInfo);
   vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(descriptorPool)
      .setSetLayouts(layouts);

   vk::Result result = device.allocateDescriptorSets(&allocateInfo, sets.data());
   ASSERT(result == vk::Result::eSuccess);

   for (std::size_t i = 0; i < sets.size(); ++i)
   {
      NAME_CHILD(sets[i], "Descriptor Set " + DebugUtils::toString(i));
   }
}
