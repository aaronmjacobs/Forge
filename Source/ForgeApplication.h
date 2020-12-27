#pragma once

#include "Core/Assert.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/UniformBuffer.h"
#include "Graphics/UniformData.h"

#include "Resources/MeshResourceManager.h"
#include "Resources/ShaderModuleResourceManager.h"
#include "Resources/TextureResourceManager.h"

#include <memory>
#include <vector>

class SimpleRenderPass;
class Swapchain;
class Window;

class ForgeApplication
{
public:
   ForgeApplication();
   ~ForgeApplication();

   void run();

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

   void initializeRenderPasses();
   void terminateRenderPasses();

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

   std::unique_ptr<Window> window;
   std::unique_ptr<GraphicsContext> context;
   std::unique_ptr<Swapchain> swapchain;

   std::unique_ptr<Texture> colorTexture;
   std::unique_ptr<Texture> depthTexture;

   std::unique_ptr<SimpleRenderPass> simpleRenderPass;

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
};
