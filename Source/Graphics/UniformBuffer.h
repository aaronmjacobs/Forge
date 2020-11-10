#pragma once

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/Context.h"
#include "Graphics/Memory.h"

#include <cstring>
#include <utility>

template<typename DataType>
class UniformBuffer
{
public:
   UniformBuffer(const VulkanContext& context, uint32_t swapchainImageCount);

   UniformBuffer(const UniformBuffer& other) = delete;
   UniformBuffer(UniformBuffer&& other);

   ~UniformBuffer();

   UniformBuffer& operator=(const UniformBuffer& other) = delete;
   UniformBuffer& operator=(UniformBuffer&& other);

private:
   void move(UniformBuffer&& other);
   void release();

public:
   void update(const VulkanContext& context, const DataType& data, uint32_t swapchainIndex);
   vk::DescriptorBufferInfo getDescriptorBufferInfo(const VulkanContext& context, uint32_t swapchainIndex) const;

private:
   static vk::DeviceSize getPaddedDataSize(const VulkanContext& context)
   {
      return Memory::getAlignedSize(static_cast<vk::DeviceSize>(sizeof(DataType)), context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
   }

   vk::Device device;

   vk::Buffer buffer;
   vk::DeviceMemory memory;
   void* mappedMemory = nullptr;
};

template<typename DataType>
inline UniformBuffer<DataType>::UniformBuffer(const VulkanContext& context, uint32_t swapchainImageCount) : device(context.device)
{
   vk::DeviceSize bufferSize = getPaddedDataSize(context) * swapchainImageCount;
   Buffer::create(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, memory);

   mappedMemory = context.device.mapMemory(memory, 0, bufferSize);
   std::memset(mappedMemory, 0, bufferSize);
}

template<typename DataType>
inline UniformBuffer<DataType>::UniformBuffer(UniformBuffer&& other)
{
   move(std::move(other));
}

template<typename DataType>
inline UniformBuffer<DataType>::~UniformBuffer()
{
   release();
}

template<typename DataType>
inline UniformBuffer<DataType>& UniformBuffer<DataType>::operator=(UniformBuffer&& other)
{
   release();
   move(std::move(other));

   return *this;
}

template<typename DataType>
inline void UniformBuffer<DataType>::move(UniformBuffer&& other)
{
   ASSERT(!device);
   device = other.device;
   other.device = nullptr;

   ASSERT(!buffer);
   buffer = other.buffer;
   other.buffer = nullptr;

   ASSERT(!memory);
   memory = other.memory;
   other.memory = nullptr;

   ASSERT(!mappedMemory);
   mappedMemory = other.mappedMemory;
   other.mappedMemory = nullptr;
}

template<typename DataType>
inline void UniformBuffer<DataType>::release()
{
   if (device)
   {
      ASSERT(memory);
      device.unmapMemory(memory);
      mappedMemory = nullptr;

      ASSERT(buffer);
      device.destroyBuffer(buffer);
      buffer = nullptr;

      ASSERT(memory);
      device.freeMemory(memory);
      memory = nullptr;
   }
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
