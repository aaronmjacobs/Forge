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

   static const vk::DispatchLoaderDynamic& GetDynamicLoader();

   GraphicsContext(Window& window);

   GraphicsContext(const GraphicsContext& other) = delete;
   GraphicsContext(GraphicsContext&& other) = delete;

   ~GraphicsContext();

   GraphicsContext& operator=(const GraphicsContext&& other) = delete;
   GraphicsContext& operator=(GraphicsContext&& other) = delete;

   vk::Instance getInstance() const
   {
      return instance;
   }

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

   vk::PipelineCache getPipelineCache() const
   {
      return pipelineCache;
   }

   VmaAllocator getVmaAllocator() const
   {
      ASSERT(vmaAllocator);
      return vmaAllocator;
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

   vk::DescriptorSetLayout getDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& createInfo) const;

   template<typename T>
   void delayedDestroy(T&& object) const
   {
      delayedDestroy(Types::bit_cast<uint64_t>(object), T::objectType);
      object = nullptr;
   }

   template<typename T>
   void delayedDestroy(T&& object, VmaAllocation&& allocation) const
   {
      delayedDestroy(Types::bit_cast<uint64_t>(object), T::objectType, allocation);
      object = nullptr;
      allocation = nullptr;
   }

   template<typename T>
   void delayedFree(T&& object) const
   {
      delayedFree(0, Types::bit_cast<uint64_t>(object), T::objectType);
      object = nullptr;
   }

   template<typename T, typename U>
   void delayedFree(U pool, T&& object) const
   {
      delayedFree(Types::bit_cast<uint64_t>(pool), Types::bit_cast<uint64_t>(object), T::objectType);
      object = nullptr;
   }

private:
   static vk::DispatchLoaderDynamic dispatchLoaderDynamic;

   void delayedDestroy(uint64_t handle, vk::ObjectType type, VmaAllocation allocation = nullptr) const;
   void delayedFree(uint64_t pool, uint64_t handle, vk::ObjectType type) const;

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
   vk::PipelineCache pipelineCache;

   VmaAllocator vmaAllocator = nullptr;

   const Swapchain* swapchain = nullptr;
   uint32_t swapchainIndex = 0;

   uint32_t frameIndex = 0;

   std::unique_ptr<DelayedObjectDestroyer> delayedObjectDestroyer;
   std::unique_ptr<DescriptorSetLayoutCache> layoutCache;

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
   PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
   PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;
#endif // FORGE_DEBUG
};
