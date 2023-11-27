#pragma once

#include "Graphics/GraphicsResource.h"

#include <array>

class DynamicDescriptorPool;

class DescriptorSet : public GraphicsResource
{
public:
   DescriptorSet(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, const vk::DescriptorSetLayoutCreateInfo& createInfo);

   vk::DescriptorSet getSet(uint32_t frameIndex) const
   {
      return sets[frameIndex];
   }

   vk::DescriptorSet getCurrentSet() const
   {
      return getSet(context.getFrameIndex());
   }

private:
   std::array<vk::DescriptorSet, GraphicsContext::kMaxFramesInFlight> sets;
};

template<typename Derived>
class TypedDescriptorSet : public DescriptorSet
{
public:
   static const vk::DescriptorSetLayoutCreateInfo& getCreateInfo()
   {
      static const auto kBindings = Derived::getBindings();
      static const auto kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags{}, kBindings);

      return kCreateInfo;
   }

   TypedDescriptorSet(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool)
      : DescriptorSet(graphicsContext, dynamicDescriptorPool, getCreateInfo())
   {
   }
};
