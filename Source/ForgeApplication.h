#pragma once

#include "Core/Assert.h"

#include "Graphics/Vulkan.h"

#include <glm/glm.hpp>

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

struct VulkanContext
{
   vk::Instance instance;
   vk::SurfaceKHR surface;
   vk::PhysicalDevice physicalDevice;
   vk::PhysicalDeviceProperties physicalDeviceProperties;
   QueueFamilyIndices queueFamilyIndices;
   vk::Device device;
   vk::Queue graphicsQueue;
   vk::Queue presentQueue;

   vk::CommandPool transientCommandPool;
};

struct Vertex
{
   glm::vec2 position;
   glm::vec3 color;

   Vertex(const glm::vec2& initialPosition = glm::vec2(0.0f), const glm::vec3& initialColor = glm::vec3(0.0f))
      : position(initialPosition)
      , color(initialColor)
   {
   }

   static vk::VertexInputBindingDescription getBindingDescription();
   static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
};

struct Mesh
{
   vk::Buffer buffer;
   vk::DeviceMemory deviceMemory;
   vk::DeviceSize indexOffset = 0;
   uint32_t numIndices = 0;

   void initialize(const VulkanContext& context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
   void terminate(const VulkanContext& context);
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

   void updateUniformBuffers(uint32_t index);

   void recreateSwapchain();

   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeSwapchain();
   void terminateSwapchain();

   void initializeRenderPass();
   void terminateRenderPass();

   void initializeDescriptorSetLayouts();
   void terminateDescriptorSetLayouts();

   void initializeGraphicsPipeline();
   void terminateGraphicsPipeline();

   void initializeFramebuffers();
   void terminateFramebuffers();

   void initializeTransientCommandPool();
   void terminateTransientCommandPool();

   void initializeUniformBuffers();
   void terminateUniformBuffers();

   void initializeDescriptorPool();
   void terminateDescriptorPool();

   void initializeDescriptorSets();
   void terminateDescriptorSets();

   void initializeMesh();
   void terminateMesh();

   void initializeCommandBuffers();
   void terminateCommandBuffers(bool keepPoolAlive);

   void initializeSyncObjects();
   void terminateSyncObjects();

   void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) const;

   GLFWwindow* window = nullptr;

   VulkanContext context;

   vk::SwapchainKHR swapchain;
   std::vector<vk::Image> swapchainImages;
   vk::Format swapchainImageFormat;
   vk::Extent2D swapchainExtent;
   std::vector<vk::ImageView> swapchainImageViews;

   vk::RenderPass renderPass;
   vk::DescriptorSetLayout frameDescriptorSetLayout;
   vk::DescriptorSetLayout drawDescriptorSetLayout;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

   vk::DescriptorPool descriptorPool;
   std::vector<vk::DescriptorSet> frameDescriptorSets;
   std::vector<vk::DescriptorSet> drawDescriptorSets;

   vk::Buffer uniformBuffers;
   vk::DeviceMemory uniformBufferMemory;
   Mesh quadMesh;

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
