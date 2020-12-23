#pragma once

#include "Graphics/Vulkan.h"

#include <optional>
#include <set>

struct GLFWwindow;

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

class GraphicsContext
{
public:
   GraphicsContext(GLFWwindow* window);

   GraphicsContext(const GraphicsContext& other) = delete;
   GraphicsContext(GraphicsContext&& other) = delete;

   ~GraphicsContext();

   GraphicsContext& operator=(const GraphicsContext&& other) = delete;
   GraphicsContext& operator=(GraphicsContext&& other) = delete;

   vk::SurfaceKHR getSurface() const
   {
      return surface;
   }

   vk::PhysicalDevice getPhysicalDevice() const
   {
      return physicalDevice;
   }

   vk::Device getDevice() const
   {
      return device;
   }

   vk::Queue getGraphicsQueue() const
   {
      return graphicsQueue;
   }

   vk::Queue getPresentQueue() const
   {
      return presentQueue;
   }

   const QueueFamilyIndices& getQueueFamilyIndices() const
   {
      return queueFamilyIndices;
   }

   const vk::PhysicalDeviceProperties& getPhysicalDeviceProperties() const
   {
      return physicalDeviceProperties;
   }

   const vk::PhysicalDeviceFeatures& getPhysicalDeviceFeatures() const
   {
      return physicalDeviceFeatures;
   }

   vk::CommandPool getTransientCommandPool() const
   {
      return transientCommandPool;
   }

private:
   vk::Instance instance;
   vk::SurfaceKHR surface;

   vk::PhysicalDevice physicalDevice;
   vk::Device device;

   vk::Queue graphicsQueue;
   vk::Queue presentQueue;
   QueueFamilyIndices queueFamilyIndices;

   vk::PhysicalDeviceProperties physicalDeviceProperties;
   vk::PhysicalDeviceFeatures physicalDeviceFeatures;

   vk::CommandPool transientCommandPool;

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
