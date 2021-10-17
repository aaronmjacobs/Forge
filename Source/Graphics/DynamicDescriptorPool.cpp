#include "Graphics/DynamicDescriptorPool.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"

namespace
{
   uint32_t getIndexForDescriptorType(vk::DescriptorType descriptorType)
   {
      switch (descriptorType)
      {
      case vk::DescriptorType::eSampler:
         return 0;
      case vk::DescriptorType::eCombinedImageSampler:
         return 1;
      case vk::DescriptorType::eSampledImage:
         return 2;
      case vk::DescriptorType::eStorageImage:
         return 3;
      case vk::DescriptorType::eUniformTexelBuffer:
         return 4;
      case vk::DescriptorType::eStorageTexelBuffer:
         return 5;
      case vk::DescriptorType::eUniformBuffer:
         return 6;
      case vk::DescriptorType::eStorageBuffer:
         return 7;
      case vk::DescriptorType::eUniformBufferDynamic:
         return 8;
      case vk::DescriptorType::eStorageBufferDynamic:
         return 9;
      case vk::DescriptorType::eInputAttachment:
         return 10;
      case vk::DescriptorType::eInlineUniformBlockEXT:
         return 11;
      case vk::DescriptorType::eAccelerationStructureKHR:
         return 12;
      case vk::DescriptorType::eAccelerationStructureNV:
         return 13;
      case vk::DescriptorType::eMutableVALVE:
         return 14;
      default:
         ASSERT(false);
         return -1;
      }
   }

   vk::DescriptorType getDescriptorTypeByIndex(uint32_t index)
   {
      switch (index)
      {
      case 0:
         return vk::DescriptorType::eSampler;
      case 1:
         return vk::DescriptorType::eCombinedImageSampler;
      case 2:
         return vk::DescriptorType::eSampledImage;
      case 3:
         return vk::DescriptorType::eStorageImage;
      case 4:
         return vk::DescriptorType::eUniformTexelBuffer;
      case 5:
         return vk::DescriptorType::eStorageTexelBuffer;
      case 6:
         return vk::DescriptorType::eUniformBuffer;
      case 7:
         return vk::DescriptorType::eStorageBuffer;
      case 8:
         return vk::DescriptorType::eUniformBufferDynamic;
      case 9:
         return vk::DescriptorType::eStorageBufferDynamic;
      case 10:
         return vk::DescriptorType::eInputAttachment;
      case 11:
         return vk::DescriptorType::eInlineUniformBlockEXT;
      case 12:
         return vk::DescriptorType::eAccelerationStructureKHR;
      case 13:
         return vk::DescriptorType::eAccelerationStructureNV;
      case 14:
         return vk::DescriptorType::eMutableVALVE;
      default:
         ASSERT(false);
         return vk::DescriptorType::eSampler;
      }
   }

   uint32_t getDescriptorCount(const DynamicDescriptorPool::Sizes& sizes, vk::DescriptorType descriptorType)
   {
      switch (descriptorType)
      {
      case vk::DescriptorType::eSampler:
         return sizes.samplerCount;
      case vk::DescriptorType::eCombinedImageSampler:
         return sizes.combinedImageSamplerCount;
      case vk::DescriptorType::eSampledImage:
         return sizes.sampledImageCount;
      case vk::DescriptorType::eStorageImage:
         return sizes.storageImageCount;
      case vk::DescriptorType::eUniformTexelBuffer:
         return sizes.uniformTexelBufferCount;
      case vk::DescriptorType::eStorageTexelBuffer:
         return sizes.storageTexelBufferCount;
      case vk::DescriptorType::eUniformBuffer:
         return sizes.uniformBufferCount;
      case vk::DescriptorType::eStorageBuffer:
         return sizes.storageBufferCount;
      case vk::DescriptorType::eUniformBufferDynamic:
         return sizes.uniformBufferDynamicCount;
      case vk::DescriptorType::eStorageBufferDynamic:
         return sizes.storageBufferDynamicCount;
      case vk::DescriptorType::eInputAttachment:
         return sizes.inputAttachmentCount;
      case vk::DescriptorType::eInlineUniformBlockEXT:
         return sizes.inlineUniformBlockEXTCount;
      case vk::DescriptorType::eAccelerationStructureKHR:
         return sizes.accelerationStructureKHRCount;
      case vk::DescriptorType::eAccelerationStructureNV:
         return sizes.accelerationStructureNVCount;
      case vk::DescriptorType::eMutableVALVE:
         return sizes.mutableVALVECount;
      default:
         ASSERT(false);
         return 0;
      }
   }
}

DynamicDescriptorPool::DynamicDescriptorPool(const GraphicsContext& graphicsContext, const Sizes& poolSizes)
   : GraphicsResource(graphicsContext)
   , sizes(poolSizes)
{
}

DynamicDescriptorPool::~DynamicDescriptorPool()
{
   for (const PoolInfo& poolInfo : pools)
   {
      device.destroyDescriptorPool(poolInfo.pool);
   }
}

vk::DescriptorPool DynamicDescriptorPool::obtainPool(const vk::DescriptorSetLayoutCreateInfo& createInfo)
{
   for (uint32_t i = 0; i < createInfo.bindingCount; ++i)
   {
      const vk::DescriptorSetLayoutBinding& binding = createInfo.pBindings[i];

      uint32_t descriptorCount = getDescriptorCount(sizes, binding.descriptorType);
      if (binding.descriptorCount > descriptorCount)
      {
         ASSERT(false, "Binding descriptor count larger than total descriptor count, allocation not possible");
         return nullptr;
      }
   }

   PoolInfo* poolInfo = findPool(createInfo);
   if (!poolInfo)
   {
      poolInfo = allocatePool();
   }
   ASSERT(poolInfo);

   ++poolInfo->usedSets;
   for (uint32_t i = 0; i < createInfo.bindingCount; ++i)
   {
      const vk::DescriptorSetLayoutBinding& binding = createInfo.pBindings[i];
      DescriptorAllocationInfo& descriptorAllocationInfo = poolInfo->descriptorAllocationInfo[getIndexForDescriptorType(binding.descriptorType)];

      ASSERT(descriptorAllocationInfo.size - descriptorAllocationInfo.used >= binding.descriptorCount);
      descriptorAllocationInfo.used += binding.descriptorCount;
   }

   return poolInfo->pool;
}

DynamicDescriptorPool::PoolInfo* DynamicDescriptorPool::findPool(const vk::DescriptorSetLayoutCreateInfo& createInfo)
{
   for (PoolInfo& poolInfo : pools)
   {
      if (poolInfo.usedSets < sizes.maxSets)
      {
         bool poolMatches = true;
         for (uint32_t i = 0; i < createInfo.bindingCount; ++i)
         {
            const vk::DescriptorSetLayoutBinding& binding = createInfo.pBindings[i];
            const DescriptorAllocationInfo& descriptorAllocationInfo = poolInfo.descriptorAllocationInfo[getIndexForDescriptorType(binding.descriptorType)];

            if (descriptorAllocationInfo.size - descriptorAllocationInfo.used < binding.descriptorCount)
            {
               // Not enough descriptors left
               poolMatches = false;
               break;
            }
         }

         if (poolMatches)
         {
            return &poolInfo;
         }
      }
   }

   return nullptr;
}

DynamicDescriptorPool::PoolInfo* DynamicDescriptorPool::allocatePool()
{
   PoolInfo& poolInfo = pools.emplace_back(PoolInfo{});

   std::vector<vk::DescriptorPoolSize> poolSizes;
   for (uint32_t i = 0; i < poolInfo.descriptorAllocationInfo.size(); ++i)
   {
      vk::DescriptorType descriptorType = getDescriptorTypeByIndex(i);
      uint32_t descriptorCount = getDescriptorCount(sizes, descriptorType);

      poolInfo.descriptorAllocationInfo[i].size = descriptorCount;
      if (descriptorCount > 0)
      {
         poolSizes.emplace_back(vk::DescriptorPoolSize().setType(descriptorType).setDescriptorCount(descriptorCount * GraphicsContext::kMaxFramesInFlight));
      }
   }

   vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
      .setPoolSizes(poolSizes)
      .setMaxSets(sizes.maxSets * GraphicsContext::kMaxFramesInFlight);
   poolInfo.pool = device.createDescriptorPool(createInfo);
   NAME_CHILD(poolInfo.pool, "Pool " + std::to_string(pools.size()));

   return &poolInfo;
}
