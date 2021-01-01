#include "Renderer/Renderer.h"

#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthPass.h"
#include "Renderer/Passes/Simple/SimpleRenderPass.h"
#include "Renderer/View.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
   vk::SampleCountFlagBits getMaxSampleCount(const GraphicsContext& context)
   {
      const vk::PhysicalDeviceLimits& limits = context.getPhysicalDeviceProperties().limits;
      vk::SampleCountFlags flags = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;

      if (flags & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
      if (flags & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
      if (flags & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
      if (flags & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
      if (flags & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
      if (flags & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

      return vk::SampleCountFlagBits::e1;
   }

   std::unique_ptr<Texture> createColorTexture(const GraphicsContext& context)
   {
      const Swapchain& swapchain = context.getSwapchain();

      ImageProperties colorImageProperties;
      colorImageProperties.format = swapchain.getFormat();
      colorImageProperties.width = swapchain.getExtent().width;
      colorImageProperties.height = swapchain.getExtent().height;

      TextureProperties colorTextureProperties;
      colorTextureProperties.sampleCount = getMaxSampleCount(context);
      colorTextureProperties.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
      colorTextureProperties.aspects = vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout colorInitialLayout;
      colorInitialLayout.layout = vk::ImageLayout::eColorAttachmentOptimal;
      colorInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

      return std::make_unique<Texture>(context, colorImageProperties, colorTextureProperties, colorInitialLayout);
   }

   std::unique_ptr<Texture> createDepthTexture(const GraphicsContext& context)
   {
      const Swapchain& swapchain = context.getSwapchain();

      ImageProperties depthImageProperties;
      depthImageProperties.format = Texture::findSupportedFormat(context, { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
      depthImageProperties.width = swapchain.getExtent().width;
      depthImageProperties.height = swapchain.getExtent().height;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = getMaxSampleCount(context);
      depthTextureProperties.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
      depthTextureProperties.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

      TextureInitialLayout depthInitialLayout;
      depthInitialLayout.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);

      return std::make_unique<Texture>(context, depthImageProperties, depthTextureProperties, depthInitialLayout);
   }
}

Renderer::Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef)
   : GraphicsResource(graphicsContext)
   , resourceManager(resourceManagerRef)
{
   {
      vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
         .setType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(GraphicsContext::kMaxFramesInFlight);

      vk::DescriptorPoolSize samplerPoolSize = vk::DescriptorPoolSize()
         .setType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(GraphicsContext::kMaxFramesInFlight);

      std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes =
      {
         uniformPoolSize,
         samplerPoolSize
      };

      vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
         .setPoolSizes(descriptorPoolSizes)
         .setMaxSets(GraphicsContext::kMaxFramesInFlight * 2);
      descriptorPool = device.createDescriptorPool(createInfo);
   }

   {
      view = std::make_unique<View>(context, descriptorPool);
   }

   {
      colorTexture = createColorTexture(context);
      depthTexture = createDepthTexture(context);
   }

   {
      depthPass = std::make_unique<DepthPass>(context, resourceManager, *depthTexture);
      simpleRenderPass = std::make_unique<SimpleRenderPass>(context, descriptorPool, resourceManager, *colorTexture, *depthTexture);
   }

   {
      meshHandle = resourceManager.loadMesh("Resources/Meshes/Viking/viking_room.obj", context);
      if (!meshHandle)
      {
         throw std::runtime_error(std::string("Failed to load mesh"));
      }

      textureHandle = resourceManager.loadTexture("Resources/Meshes/Viking/viking_room.png", context);
      if (!textureHandle)
      {
         throw std::runtime_error(std::string("Failed to load image"));
      }
   }

   {
      depthPass->updateDescriptorSets(*view);
      simpleRenderPass->updateDescriptorSets(*view, *resourceManager.getTexture(textureHandle));
   }
}

Renderer::~Renderer()
{
   resourceManager.unloadMesh(meshHandle);
   meshHandle.reset();

   resourceManager.unloadTexture(textureHandle);
   textureHandle.reset();

   simpleRenderPass = nullptr;
   depthPass = nullptr;

   depthTexture = nullptr;
   colorTexture = nullptr;

   view = nullptr;

   device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void Renderer::render(vk::CommandBuffer commandBuffer)
{
   view->update();

   const Mesh* mesh = resourceManager.getMesh(meshHandle);
   ASSERT(mesh);

   float time = static_cast<float>(glfwGetTime());
   glm::mat4 localToWorld = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * time, glm::vec3(0.0f, 0.0f, 1.0f));

   depthPass->render(commandBuffer, *view, *mesh, localToWorld);
   simpleRenderPass->render(commandBuffer, *view, *mesh, localToWorld);
}

void Renderer::onSwapchainRecreated()
{
   colorTexture = createColorTexture(context);
   depthTexture = createDepthTexture(context);

   depthPass->onSwapchainRecreated(*depthTexture);
   simpleRenderPass->onSwapchainRecreated(*colorTexture, *depthTexture);
}
