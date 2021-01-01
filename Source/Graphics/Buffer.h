#pragma once

#include "Graphics/GraphicsContext.h"

#include <span>

namespace Buffer
{
   struct CopyInfo
   {
      vk::Buffer srcBuffer;
      vk::Buffer dstBuffer;

      vk::DeviceSize srcOffset = 0;
      vk::DeviceSize dstOffset = 0;

      vk::DeviceSize size = 0;
   };

   void create(const GraphicsContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
   void copy(const GraphicsContext& context, std::span<const CopyInfo> copyInfo);
}
