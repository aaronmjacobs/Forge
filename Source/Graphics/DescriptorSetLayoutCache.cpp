#include "Graphics/DescriptorSetLayoutCache.h"

#include "Core/Enum.h"
#include "Core/Hash.h"

namespace std
{
   size_t hash<vk::DescriptorSetLayoutBinding>::operator()(const vk::DescriptorSetLayoutBinding& value) const
   {
      size_t seed = 0;

      Hash::combine(seed, value.binding);
      Hash::combine(seed, Enum::cast(value.descriptorType));
      Hash::combine(seed, value.descriptorCount);
      Hash::combine(seed, static_cast<vk::ShaderStageFlags::MaskType>(value.stageFlags));
      if (value.pImmutableSamplers)
      {
         for (uint32_t i = 0; i < value.descriptorCount; ++i)
         {
            Hash::combine(seed, value.pImmutableSamplers[i]);
         }
      }

      return seed;
   }

   size_t hash<vk::DescriptorSetLayoutCreateInfo>::operator()(const vk::DescriptorSetLayoutCreateInfo& value) const
   {
      size_t seed = 0;

      Hash::combine(seed, Enum::cast(value.sType));
      Hash::combine(seed, value.pNext);
      Hash::combine(seed, static_cast<vk::DescriptorSetLayoutCreateFlags::MaskType>(value.flags));
      Hash::combine(seed, value.bindingCount);
      for (uint32_t i = 0; i < value.bindingCount; ++i)
      {
         Hash::combine(seed, value.pBindings[i]);
      }

      return seed;
   }
}

DescriptorSetLayoutCache::DescriptorSetLayoutCache(const GraphicsContext& graphicsContext)
   : GraphicsResource(graphicsContext)
{
}

DescriptorSetLayoutCache::~DescriptorSetLayoutCache()
{
   for (const auto& pair : layoutMap)
   {
      vk::DescriptorSetLayout layout = pair.second;
      device.destroyDescriptorSetLayout(layout);
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
