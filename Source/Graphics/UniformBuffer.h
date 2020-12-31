#pragma once

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Memory.h"
#include "Graphics/Swapchain.h"

#include <cstring>
#include <utility>

template<typename DataType>
class UniformBuffer : public GraphicsResource
{
public:
   UniformBuffer(const GraphicsContext& context);

   ~UniformBuffer();

   void update(const DataType& data);
   vk::DescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const;

private:
   static vk::DeviceSize getPaddedDataSize(const GraphicsContext& context)
   {
      return Memory::getAlignedSize(static_cast<vk::DeviceSize>(sizeof(DataType)), context.getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
   }

   vk::Buffer buffer;
   vk::DeviceMemory memory;
   void* mappedMemory = nullptr;
};

template<typename DataType>
inline UniformBuffer<DataType>::UniformBuffer(const GraphicsContext& graphicsContext)
   : GraphicsResource(graphicsContext)
{
   vk::DeviceSize bufferSize = getPaddedDataSize(context) * GraphicsContext::kMaxFramesInFlight;
   Buffer::create(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, memory);

   mappedMemory = device.mapMemory(memory, 0, bufferSize);
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
inline void UniformBuffer<DataType>::update(const DataType& data)
{
   ASSERT(buffer && memory && mappedMemory);

   vk::DeviceSize size = getPaddedDataSize(context);
   vk::DeviceSize offset = size * context.getFrameIndex();

   std::memcpy(static_cast<char*>(mappedMemory) + offset, &data, sizeof(data));
}

template<typename DataType>
inline vk::DescriptorBufferInfo UniformBuffer<DataType>::getDescriptorBufferInfo(uint32_t frameIndex) const
{
   return vk::DescriptorBufferInfo()
      .setBuffer(buffer)
      .setOffset(static_cast<vk::DeviceSize>(getPaddedDataSize(context) * frameIndex))
      .setRange(sizeof(DataType));
}
