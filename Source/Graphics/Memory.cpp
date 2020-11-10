#include "Graphics/Memory.h"

namespace Memory
{
   uint32_t findType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
   {
      vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();

      for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
      {
         if ((typeFilter & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
         {
            return i;
         }
      }

      throw std::runtime_error("Failed to find suitable memory type");
   }
}
