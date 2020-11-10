#pragma once

#include "Graphics/Vulkan.h"

#include <optional>
#include <set>

struct QueueFamilyIndices
{
   uint32_t graphicsFamily = 0;
   uint32_t presentFamily = 0;

   static std::optional<QueueFamilyIndices> get(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

   std::set<uint32_t> getUniqueIndices() const
   {
      return { graphicsFamily, presentFamily };
   }
};

struct VulkanContext
{
   vk::Instance instance;
   vk::SurfaceKHR surface;
   vk::PhysicalDevice physicalDevice;
   vk::Device device;
   vk::Queue graphicsQueue;
   vk::Queue presentQueue;

   vk::PhysicalDeviceProperties physicalDeviceProperties;
   vk::PhysicalDeviceFeatures physicalDeviceFeatures;
   QueueFamilyIndices queueFamilyIndices;

   vk::CommandPool transientCommandPool;
};
