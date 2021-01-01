#include "Graphics/Buffer.h"

#include "Graphics/Command.h"
#include "Graphics/Memory.h"

namespace Buffer
{
   void create(const GraphicsContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
   {
      vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
         .setSize(size)
         .setUsage(usage)
         .setSharingMode(vk::SharingMode::eExclusive);

      buffer = context.getDevice().createBuffer(bufferCreateInfo);

      vk::MemoryRequirements memoryRequirements = context.getDevice().getBufferMemoryRequirements(buffer);
      vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
         .setAllocationSize(memoryRequirements.size)
         .setMemoryTypeIndex(Memory::findType(context.getPhysicalDevice(), memoryRequirements.memoryTypeBits, properties));

      bufferMemory = context.getDevice().allocateMemory(allocateInfo);
      context.getDevice().bindBufferMemory(buffer, bufferMemory, 0);
   }

   void copy(const GraphicsContext& context, std::span<const CopyInfo> copyInfo)
   {
      vk::CommandBuffer copyCommandBuffer = Command::beginSingle(context);

      for (const CopyInfo& info : copyInfo)
      {
         vk::BufferCopy copyRegion = vk::BufferCopy()
            .setSrcOffset(info.srcOffset)
            .setDstOffset(info.dstOffset)
            .setSize(info.size);
         copyCommandBuffer.copyBuffer(info.srcBuffer, info.dstBuffer, { copyRegion });
      }

      Command::endSingle(context, copyCommandBuffer);
   }
}
