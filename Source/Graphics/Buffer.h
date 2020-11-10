#pragma once

#include "Graphics/Context.h"

#include <vector>

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

   void create(const VulkanContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
   void copy(const VulkanContext& context, const std::vector<CopyInfo>& copyInfo);
}
