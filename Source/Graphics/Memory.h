#pragma once

#include "Graphics/Context.h"

namespace Memory
{
   inline vk::DeviceSize getAlignedSize(vk::DeviceSize size, vk::DeviceSize alignment)
   {
      ASSERT((alignment & (alignment - 1)) == 0, "Alignment is not a power of two: %llu", alignment);
      return (size + (alignment - 1)) & ~(alignment - 1);
   }

   uint32_t findType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
}
