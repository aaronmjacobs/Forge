#pragma once

#include "Core/Hash.h"

#include "Graphics/GraphicsResource.h"

#include <unordered_map>

namespace std
{
   template<>
   struct hash<vk::DescriptorSetLayoutBinding>
   {
      size_t operator()(const vk::DescriptorSetLayoutBinding& value) const;
   };

   template<>
   struct hash<vk::DescriptorSetLayoutCreateInfo>
   {
      size_t operator()(const vk::DescriptorSetLayoutCreateInfo& value) const;
   };
}

class DescriptorSetLayoutCache : public GraphicsResource
{
public:
   DescriptorSetLayoutCache(const GraphicsContext& graphicsContext);

   ~DescriptorSetLayoutCache();

   vk::DescriptorSetLayout getLayout(const vk::DescriptorSetLayoutCreateInfo& createInfo);

private:
   std::unordered_map<vk::DescriptorSetLayoutCreateInfo, vk::DescriptorSetLayout> layoutMap;
};
