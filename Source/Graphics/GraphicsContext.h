#pragma once

#include "Core/Types.h"

#include "Graphics/Vulkan.h"

#include <memory>
#include <optional>
#include <set>

class DescriptorSetLayoutCache;
class DelayedObjectDestroyer;
class Swapchain;
class Window;

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
   static const uint32_t kMaxFramesInFlight = 3;

   GraphicsContext(Window& window);

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

   const Swapchain& getSwapchain() const
   {
      ASSERT(swapchain);
      return *swapchain;
   }

   void setSwapchain(const Swapchain* newSwapchain)
   {
      swapchain = newSwapchain;
   }

   uint32_t getSwapchainIndex() const
   {
      return swapchainIndex;
   }

   void setSwapchainIndex(uint32_t index);

   uint32_t getFrameIndex() const
   {
      return frameIndex;
   }

   void setFrameIndex(uint32_t index);

   DescriptorSetLayoutCache& getLayoutCache() const
   {
      ASSERT(layoutCache);
      return *layoutCache;
   }

   template<typename T>
   void delayedDestroy(T&& object) const
   {
      delayedDestroy(Types::bit_cast<uint64_t>(object), T::objectType);
      object = nullptr;
   }

private:
   void delayedDestroy(uint64_t handle, vk::ObjectType type) const;

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

   const Swapchain* swapchain = nullptr;
   uint32_t swapchainIndex = 0;

   uint32_t frameIndex = 0;

   std::unique_ptr<DescriptorSetLayoutCache> layoutCache;
   std::unique_ptr<DelayedObjectDestroyer> delayedObjectDestroyer;

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
