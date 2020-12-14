#pragma once

#include "Core/Assert.h"

#include "Graphics/Context.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/UniformBuffer.h"

#include "Resources/MeshResourceManager.h"
#include "Resources/ShaderModuleResourceManager.h"
#include "Resources/TextureResourceManager.h"

#include <glm/glm.hpp>

#include <optional>
#include <vector>

struct GLFWwindow;

struct ViewUniformData
{
   alignas(16) glm::mat4 worldToClip;
};

struct MeshUniformData
{
   alignas(16) glm::mat4 localToWorld;
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

   MeshResourceManager meshResourceManager;
   ShaderModuleResourceManager shaderModuleResourceManager;
   TextureResourceManager textureResourceManager;

   GLFWwindow* window = nullptr;

   VulkanContext context;

   vk::SwapchainKHR swapchain;
   std::vector<vk::Image> swapchainImages;
   vk::Format swapchainImageFormat;
   vk::Extent2D swapchainExtent;
   std::vector<vk::ImageView> swapchainImageViews;

   vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
   std::optional<Texture> colorTexture;

   std::optional<Texture> depthTexture;

   vk::Sampler sampler;

   vk::RenderPass renderPass;
   vk::DescriptorSetLayout frameDescriptorSetLayout;
   vk::DescriptorSetLayout drawDescriptorSetLayout;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

   vk::DescriptorPool descriptorPool;
   std::vector<vk::DescriptorSet> frameDescriptorSets;
   std::vector<vk::DescriptorSet> drawDescriptorSets;

   std::optional<UniformBuffer<ViewUniformData>> viewUniformBuffer;
   std::optional<UniformBuffer<MeshUniformData>> meshUniformBuffer;

   TextureHandle textureHandle;
   MeshHandle meshHandle;

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
