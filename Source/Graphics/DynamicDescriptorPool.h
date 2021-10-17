#pragma once

#include "Graphics/GraphicsResource.h"

#include <array>
#include <vector>

class DynamicDescriptorPool : public GraphicsResource
{
public:
   struct Sizes
   {
      uint32_t maxSets = 0;

      uint32_t samplerCount = 0;
      uint32_t combinedImageSamplerCount = 0;
      uint32_t sampledImageCount = 0;
      uint32_t storageImageCount = 0;
      uint32_t uniformTexelBufferCount = 0;
      uint32_t storageTexelBufferCount = 0;
      uint32_t uniformBufferCount = 0;
      uint32_t storageBufferCount = 0;
      uint32_t uniformBufferDynamicCount = 0;
      uint32_t storageBufferDynamicCount = 0;
      uint32_t inputAttachmentCount = 0;
      uint32_t inlineUniformBlockEXTCount = 0;
      uint32_t accelerationStructureKHRCount = 0;
      uint32_t accelerationStructureNVCount = 0;
      uint32_t mutableVALVECount = 0;
   };

   DynamicDescriptorPool(const GraphicsContext& graphicsContext, const Sizes& poolSizes);
   ~DynamicDescriptorPool();

   vk::DescriptorPool obtainPool(const vk::DescriptorSetLayoutCreateInfo& createInfo);

private:
   struct DescriptorAllocationInfo
   {
      uint32_t size = 0;
      uint32_t used = 0;
   };

   struct PoolInfo
   {
      std::array<DescriptorAllocationInfo, 15> descriptorAllocationInfo;
      uint32_t usedSets = 0;
      vk::DescriptorPool pool;
   };

   PoolInfo* findPool(const vk::DescriptorSetLayoutCreateInfo& createInfo);
   PoolInfo* allocatePool();

   const Sizes sizes;
   std::vector<PoolInfo> pools;
};
