#pragma once

#include "Core/Assert.h"

#include "Graphics/Vulkan.h"

#include <optional>
#include <set>
#include <vector>

struct GLFWwindow;

struct QueueFamilyIndices
{
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool isComplete() const
   {
      return graphicsFamily && presentFamily;
   }

   std::set<uint32_t> getUniqueIndices() const
   {
      ASSERT(isComplete());

      return { *graphicsFamily, *presentFamily };
   }
};

class ForgeApplication
{
public:
   ForgeApplication();
   ~ForgeApplication();

   void run();

   void onFramebufferResized()
   {
      framebufferResized = true;
   }

private:
   void render();

   void recreateSwapchain();

   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeSwapchain();
   void terminateSwapchain();

   void initializeRenderPass();
   void terminateRenderPass();

   void initializeGraphicsPipeline();
   void terminateGraphicsPipeline();

   void initializeFramebuffers();
   void terminateFramebuffers();

   void initializeTransientCommandPool();
   void terminateTransientCommandPool();

   void initializeVertexBuffers();
   void terminateVertexBuffers();

   void initializeCommandBuffers();
   void terminateCommandBuffers(bool keepPoolAlive);

   void initializeSyncObjects();
   void terminateSyncObjects();

   void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) const;
   void copyBuffer(vk::Buffer sourceBuffer, vk::Buffer destinationBuffer, vk::DeviceSize size);

   GLFWwindow* window = nullptr;

   vk::Instance instance;
   vk::SurfaceKHR surface;
   vk::PhysicalDevice physicalDevice;
   QueueFamilyIndices queueFamilyIndices;
   vk::Device device;
   vk::Queue graphicsQueue;
   vk::Queue presentQueue;

   vk::SwapchainKHR swapchain;
   std::vector<vk::Image> swapchainImages;
   vk::Format swapchainImageFormat;
   vk::Extent2D swapchainExtent;
   std::vector<vk::ImageView> swapchainImageViews;

   vk::RenderPass renderPass;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

   vk::CommandPool transientCommandPool;

   vk::Buffer vertexBuffer;
   vk::DeviceMemory vertexBufferMemory;

   vk::CommandPool commandPool;
   std::vector<vk::CommandBuffer> commandBuffers;

   std::vector<vk::Semaphore> imageAvailableSemaphores;
   std::vector<vk::Semaphore> renderFinishedSemaphores;
   std::vector<vk::Fence> frameFences;
   std::vector<vk::Fence> imageFences;
   std::size_t frameIndex = 0;

   bool framebufferResized = false;

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
