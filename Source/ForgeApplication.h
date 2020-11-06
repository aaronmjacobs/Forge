#pragma once

#include "Core/Assert.h"

#include "Graphics/Vulkan.h"

#include <glm/glm.hpp>

#include <optional>
#include <set>
#include <vector>

struct GLFWwindow;

namespace
{
   inline vk::DeviceSize getAlignedSize(vk::DeviceSize size, vk::DeviceSize alignment)
   {
      ASSERT((alignment & (alignment - 1)) == 0, "Alignment is not a power of two: %llu", alignment);
      return (size + (alignment - 1)) & ~(alignment - 1);
   }
}

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
   vk::PhysicalDeviceFeatures physicalDeviceFeatures;
   QueueFamilyIndices queueFamilyIndices;
   vk::Device device;
   vk::Queue graphicsQueue;
   vk::Queue presentQueue;

   vk::CommandPool transientCommandPool;
};

template<typename DataType>
class UniformBuffer
{
public:
   void initialize(const VulkanContext& context, uint32_t swapchainImageCount)
   {
      ASSERT(!buffer && !memory && !mappedMemory);

      vk::DeviceSize bufferSize = getPaddedDataSize(context) * swapchainImageCount;
      createBuffer(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, memory);

      mappedMemory = context.device.mapMemory(memory, 0, bufferSize);
      std::memset(mappedMemory, 0, bufferSize);
   }

   void terminate(const VulkanContext& context)
   {
      ASSERT(buffer && memory && mappedMemory);

      context.device.unmapMemory(memory);
      mappedMemory = nullptr;

      context.device.destroyBuffer(buffer);
      buffer = nullptr;

      context.device.freeMemory(memory);
      memory = nullptr;
   }

   void update(const VulkanContext& context, const DataType& data, uint32_t swapchainIndex)
   {
      ASSERT(buffer && memory && mappedMemory);

      vk::DeviceSize size = getPaddedDataSize(context);
      vk::DeviceSize offset = size * swapchainIndex;

      std::memcpy(static_cast<char*>(mappedMemory) + offset, &data, sizeof(data));
   }

   vk::DescriptorBufferInfo getDescriptorBufferInfo(const VulkanContext& context, uint32_t swapchainIndex) const
   {
      return vk::DescriptorBufferInfo()
         .setBuffer(buffer)
         .setOffset(static_cast<vk::DeviceSize>(getPaddedDataSize(context) * swapchainIndex))
         .setRange(sizeof(DataType));
   }

private:
   static vk::DeviceSize getPaddedDataSize(const VulkanContext& context)
   {
      vk::DeviceSize alignment = context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
      return getAlignedSize(static_cast<vk::DeviceSize>(sizeof(DataType)), alignment);
   }

   vk::Buffer buffer;
   vk::DeviceMemory memory;
   void* mappedMemory = nullptr;
};

struct ViewUniformData
{
   alignas(16) glm::mat4 worldToClip;
};

struct MeshUniformData
{
   alignas(16) glm::mat4 localToWorld;
};

struct Vertex
{
   glm::vec3 position;
   glm::vec3 color;
   glm::vec2 texCoord;

   Vertex(const glm::vec3& initialPosition = glm::vec3(0.0f), const glm::vec3& initialColor = glm::vec3(0.0f), const glm::vec2& initialTexCoord = glm::vec2(0.0f))
      : position(initialPosition)
      , color(initialColor)
      , texCoord(initialTexCoord)
   {
   }

   static vk::VertexInputBindingDescription getBindingDescription();
   static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
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

struct ImageDataDeleter
{
   void operator()(uint8_t* data) const;
};

struct LoadedImage
{
   std::unique_ptr<uint8_t, ImageDataDeleter> data;
   vk::Format format = vk::Format::eR8G8B8A8Srgb;
   vk::DeviceSize size = 0;
   uint32_t width = 0;
   uint32_t height = 0;
};

struct Texture
{
   vk::Image image;
   vk::DeviceMemory memory;
   vk::ImageView view;
   vk::Sampler sampler;

   void initialize(const VulkanContext& context, const LoadedImage& loadedImage);
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

   GLFWwindow* window = nullptr;

   VulkanContext context;

   vk::SwapchainKHR swapchain;
   std::vector<vk::Image> swapchainImages;
   vk::Format swapchainImageFormat;
   vk::Extent2D swapchainExtent;
   std::vector<vk::ImageView> swapchainImageViews;

   vk::Format depthImageFormat;
   vk::DeviceMemory depthImageMemory;
   vk::Image depthImage;
   vk::ImageView depthImageView;

   vk::RenderPass renderPass;
   vk::DescriptorSetLayout frameDescriptorSetLayout;
   vk::DescriptorSetLayout drawDescriptorSetLayout;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

   vk::DescriptorPool descriptorPool;
   std::vector<vk::DescriptorSet> frameDescriptorSets;
   std::vector<vk::DescriptorSet> drawDescriptorSets;

   UniformBuffer<ViewUniformData> viewUniformBuffer;
   UniformBuffer<MeshUniformData> meshUniformBuffer;

   Texture texture;
   Mesh mesh;

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
