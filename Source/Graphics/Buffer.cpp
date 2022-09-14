#include "Graphics/Buffer.h"

#include "Graphics/Command.h"
#include "Graphics/Memory.h"

namespace Buffer
{
   void create(const GraphicsContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags, vk::Buffer& buffer, VmaAllocation& allocation, void** mappedData)
   {
      vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
         .setSize(size)
         .setUsage(usage)
         .setSharingMode(vk::SharingMode::eExclusive);

      VmaAllocationCreateInfo bufferAllocationCreateInfo{};
      bufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
      bufferAllocationCreateInfo.flags = flags;

      VkBuffer vkBuffer = nullptr;
      VmaAllocationInfo allocationInfo{};
      VkResult bufferCreateResult = vmaCreateBuffer(context.getVmaAllocator(), &static_cast<VkBufferCreateInfo&>(bufferCreateInfo), &bufferAllocationCreateInfo, &vkBuffer, &allocation, &allocationInfo);
      if (bufferCreateResult != VK_SUCCESS)
      {
         throw new std::runtime_error("Failed to allocate buffer");
      }
      buffer = vkBuffer;

      if (mappedData)
      {
         *mappedData = allocationInfo.pMappedData;
      }
   }

   void copy(const GraphicsContext& context, std::span<const CopyInfo> copyInfo)
   {
      Command::executeSingle(context, [copyInfo](vk::CommandBuffer commandBuffer)
      {
         for (const CopyInfo& info : copyInfo)
         {
            vk::BufferCopy copyRegion = vk::BufferCopy()
               .setSrcOffset(info.srcOffset)
               .setDstOffset(info.dstOffset)
               .setSize(info.size);
            commandBuffer.copyBuffer(info.srcBuffer, info.dstBuffer, { copyRegion });
         }
      });
   }
}
