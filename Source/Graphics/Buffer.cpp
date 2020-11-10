#include "Graphics/Buffer.h"

#include "Graphics/Command.h"
#include "Graphics/Memory.h"

namespace Buffer
{
   void create(const VulkanContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
   {
      vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
         .setSize(size)
         .setUsage(usage)
         .setSharingMode(vk::SharingMode::eExclusive);

      buffer = context.device.createBuffer(bufferCreateInfo);

      vk::MemoryRequirements memoryRequirements = context.device.getBufferMemoryRequirements(buffer);
      vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
         .setAllocationSize(memoryRequirements.size)
         .setMemoryTypeIndex(Memory::findType(context.physicalDevice, memoryRequirements.memoryTypeBits, properties));

      bufferMemory = context.device.allocateMemory(allocateInfo);
      context.device.bindBufferMemory(buffer, bufferMemory, 0);
   }

   void copy(const VulkanContext& context, const std::vector<CopyInfo>& copyInfo)
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
