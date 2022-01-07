#pragma once

#include "Graphics/GraphicsContext.h"

namespace DescriptorSetLayout
{
   template<typename T>
   static const vk::DescriptorSetLayoutCreateInfo& getCreateInfo()
   {
      static const auto kBindings = T::getBindings();
      static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), kBindings);

      return kCreateInfo;
   }

   template<typename T>
   static vk::DescriptorSetLayout get(const GraphicsContext& context)
   {
      return context.getDescriptorSetLayout(getCreateInfo<T>());
   }
}
