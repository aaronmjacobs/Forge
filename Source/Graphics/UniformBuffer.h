#pragma once

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/DebugUtils.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Memory.h"

#include <utility>

template<typename DataType>
class UniformBuffer : public GraphicsResource
{
public:
   UniformBuffer(const GraphicsContext& context);
   UniformBuffer(UniformBuffer&& other);
   ~UniformBuffer();

   void update(const DataType& value);
   void updateAll(const DataType& value);

   template<typename MemberType>
   void updateMember(MemberType DataType::* member, const MemberType& value);

   template<typename MemberType>
   void updateAllMembers(MemberType DataType::* member, const MemberType& value);

   vk::DescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const;

private:
   static vk::DeviceSize getPaddedDataSize(const GraphicsContext& context)
   {
      return Memory::getAlignedSize(static_cast<vk::DeviceSize>(sizeof(DataType)), context.getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
   }

   DataType* getMappedData(uint32_t index);

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
   NAME_CHILD(buffer, "Uniform Buffer");

   mappedMemory = device.mapMemory(memory, 0, bufferSize);
   std::memset(mappedMemory, 0, bufferSize);
   NAME_CHILD(memory, "Uniform Buffer Memory");
}

template<typename DataType>
inline UniformBuffer<DataType>::UniformBuffer(UniformBuffer&& other)
   : GraphicsResource(other.context)
   , buffer(other.buffer)
   , memory(other.memory)
   , mappedMemory(other.mappedMemory)
{
   other.buffer = nullptr;
   other.memory = nullptr;
   other.mappedMemory = nullptr;
}

template<typename DataType>
inline UniformBuffer<DataType>::~UniformBuffer()
{
   if (memory)
   {
      device.unmapMemory(memory);
   }

   if (buffer)
   {
      context.delayedDestroy(std::move(buffer));
   }

   if (memory)
   {
      context.delayedFree(std::move(memory));
   }
}

template<typename DataType>
inline void UniformBuffer<DataType>::update(const DataType& value)
{
   DataType* mappedData = getMappedData(context.getFrameIndex());
   *mappedData = value;
}

template<typename DataType>
inline void UniformBuffer<DataType>::updateAll(const DataType& value)
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      DataType* mappedData = getMappedData(frameIndex);
      *mappedData = value;
   }
}

template<typename DataType> template<typename MemberType>
void UniformBuffer<DataType>::updateMember(MemberType DataType::* member, const MemberType& value)
{
   ASSERT(member);

   DataType* mappedData = getMappedData(context.getFrameIndex());
   mappedData->*member = value;
}

template<typename DataType> template<typename MemberType>
void UniformBuffer<DataType>::updateAllMembers(MemberType DataType::* member, const MemberType& value)
{
   ASSERT(member);

   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      DataType* mappedData = getMappedData(frameIndex);
      mappedData->*member = value;
   }
}

template<typename DataType>
inline vk::DescriptorBufferInfo UniformBuffer<DataType>::getDescriptorBufferInfo(uint32_t frameIndex) const
{
   return vk::DescriptorBufferInfo()
      .setBuffer(buffer)
      .setOffset(static_cast<vk::DeviceSize>(getPaddedDataSize(context) * frameIndex))
      .setRange(sizeof(DataType));
}

template<typename DataType>
DataType* UniformBuffer<DataType>::getMappedData(uint32_t index)
{
   ASSERT(buffer && memory && mappedMemory);
   ASSERT(index < GraphicsContext::kMaxFramesInFlight);

   vk::DeviceSize size = getPaddedDataSize(context);
   vk::DeviceSize offset = size * index;

   return reinterpret_cast<DataType*>(static_cast<char*>(mappedMemory) + offset);
}
