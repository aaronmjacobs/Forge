#include "Graphics/DescriptorSetLayoutCache.h"

#include "Core/Enum.h"
#include "Core/Hash.h"

#include <utility>

namespace std
{
   size_t hash<vk::DescriptorSetLayoutBinding>::operator()(const vk::DescriptorSetLayoutBinding& value) const
   {
      size_t hash = 0;

      Hash::combine(hash, value.binding);
      Hash::combine(hash, Enum::cast(value.descriptorType));
      Hash::combine(hash, value.descriptorCount);
      Hash::combine(hash, static_cast<vk::ShaderStageFlags::MaskType>(value.stageFlags));
      if (value.pImmutableSamplers)
      {
         for (uint32_t i = 0; i < value.descriptorCount; ++i)
         {
            Hash::combine(hash, static_cast<VkSampler>(value.pImmutableSamplers[i]));
         }
      }

      return hash;
   }

   size_t hash<vk::DescriptorSetLayoutCreateInfo>::operator()(const vk::DescriptorSetLayoutCreateInfo& value) const
   {
      size_t hash = 0;

      Hash::combine(hash, Enum::cast(value.sType));
      Hash::combine(hash, value.pNext);
      Hash::combine(hash, static_cast<vk::DescriptorSetLayoutCreateFlags::MaskType>(value.flags));
      Hash::combine(hash, value.bindingCount);
      for (uint32_t i = 0; i < value.bindingCount; ++i)
      {
         Hash::combine(hash, value.pBindings[i]);
      }

      return hash;
   }
}

DescriptorSetLayoutCache::DescriptorSetLayoutCache(const GraphicsContext& graphicsContext)
   : GraphicsResource(graphicsContext)
{
}

DescriptorSetLayoutCache::~DescriptorSetLayoutCache()
{
   for (auto& pair : layoutMap)
   {
      vk::DescriptorSetLayout& layout = pair.second;
      context.delayedDestroy(std::move(layout));
   }
}

vk::DescriptorSetLayout DescriptorSetLayoutCache::getLayout(const vk::DescriptorSetLayoutCreateInfo& createInfo)
{
   auto location = layoutMap.find(createInfo);
   if (location != layoutMap.end())
   {
      return location->second;
   }

   vk::DescriptorSetLayout layout = device.createDescriptorSetLayout(createInfo);
   layoutMap[createInfo] = layout;

   return layout;
}
