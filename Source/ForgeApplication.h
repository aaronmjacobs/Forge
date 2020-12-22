#pragma once

#include "Core/Assert.h"

#include "Graphics/Context.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/UniformBuffer.h"
#include "Graphics/UniformData.h"
#include "Graphics/Shaders/SimpleShader.h"

#include "Resources/MeshResourceManager.h"
#include "Resources/ShaderModuleResourceManager.h"
#include "Resources/TextureResourceManager.h"

#include <memory>
#include <vector>

class Swapchain;
struct GLFWwindow;

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

   bool recreateSwapchain();

   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeSwapchain();
   void terminateSwapchain();

   void initializeRenderPass();
   void terminateRenderPass();

   void initializeShaders();
   void terminateShaders();

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

   vk::Extent2D getWindowExtent() const;

   MeshResourceManager meshResourceManager;
   ShaderModuleResourceManager shaderModuleResourceManager;
   TextureResourceManager textureResourceManager;

   GLFWwindow* window = nullptr;

   VulkanContext context;

   std::unique_ptr<Swapchain> swapchain;

   std::unique_ptr<Texture> colorTexture;
   std::unique_ptr<Texture> depthTexture;

   vk::Sampler sampler;

   std::unique_ptr<SimpleShader> simpleShader;

   vk::RenderPass renderPass;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

   vk::DescriptorPool descriptorPool;

   std::unique_ptr<UniformBuffer<ViewUniformData>> viewUniformBuffer;
   std::unique_ptr<UniformBuffer<MeshUniformData>> meshUniformBuffer;

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
