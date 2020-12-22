#pragma once

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/Context.h"
#include "Graphics/Memory.h"

#include <cstring>
#include <utility>

template<typename DataType>
class UniformBuffer : public GraphicsResource
{
public:
   UniformBuffer(const VulkanContext& context, uint32_t swapchainImageCount);

   ~UniformBuffer();

   void update(const VulkanContext& context, const DataType& data, uint32_t swapchainIndex);
   vk::DescriptorBufferInfo getDescriptorBufferInfo(const VulkanContext& context, uint32_t swapchainIndex) const;

private:
   static vk::DeviceSize getPaddedDataSize(const VulkanContext& context)
   {
      return Memory::getAlignedSize(static_cast<vk::DeviceSize>(sizeof(DataType)), context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
   }

   vk::Buffer buffer;
   vk::DeviceMemory memory;
   void* mappedMemory = nullptr;
};

template<typename DataType>
inline UniformBuffer<DataType>::UniformBuffer(const VulkanContext& context, uint32_t swapchainImageCount)
   : GraphicsResource(context)
{
   vk::DeviceSize bufferSize = getPaddedDataSize(context) * swapchainImageCount;
   Buffer::create(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, memory);

   mappedMemory = context.device.mapMemory(memory, 0, bufferSize);
   std::memset(mappedMemory, 0, bufferSize);
}

template<typename DataType>
inline UniformBuffer<DataType>::~UniformBuffer()
{
   ASSERT(memory);
   device.unmapMemory(memory);

   ASSERT(buffer);
   device.destroyBuffer(buffer);

   ASSERT(memory);
   device.freeMemory(memory);
}

template<typename DataType>
inline void UniformBuffer<DataType>::update(const VulkanContext& context, const DataType& data, uint32_t swapchainIndex)
{
   ASSERT(buffer && memory && mappedMemory);

   vk::DeviceSize size = getPaddedDataSize(context);
   vk::DeviceSize offset = size * swapchainIndex;

   std::memcpy(static_cast<char*>(mappedMemory) + offset, &data, sizeof(data));
}

template<typename DataType>
inline vk::DescriptorBufferInfo UniformBuffer<DataType>::getDescriptorBufferInfo(const VulkanContext& context, uint32_t swapchainIndex) const
{
   return vk::DescriptorBufferInfo()
      .setBuffer(buffer)
      .setOffset(static_cast<vk::DeviceSize>(getPaddedDataSize(context) * swapchainIndex))
      .setRange(sizeof(DataType));
}
