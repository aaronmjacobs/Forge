#include "Renderer/Renderer.h"

#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Simple/SimpleRenderPass.h"

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
         .setDescriptorCount(GraphicsContext::kMaxFramesInFlight * 2);

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
      colorTexture = createColorTexture(context);
      depthTexture = createDepthTexture(context);
   }

   {
      simpleRenderPass = std::make_unique<SimpleRenderPass>(context, resourceManager, *colorTexture, *depthTexture);
   }

   {
      viewUniformBuffer = std::make_unique<UniformBuffer<ViewUniformData>>(context);
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
      simpleRenderPass->allocateDescriptorSets(descriptorPool);
      simpleRenderPass->updateDescriptorSets(*viewUniformBuffer, *resourceManager.getTexture(textureHandle));
   }
}

Renderer::~Renderer()
{
   simpleRenderPass->clearDescriptorSets();

   resourceManager.unloadMesh(meshHandle);
   meshHandle.reset();

   resourceManager.unloadTexture(textureHandle);
   textureHandle.reset();

   viewUniformBuffer.reset();

   simpleRenderPass = nullptr;

   depthTexture = nullptr;
   colorTexture = nullptr;

   device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void Renderer::render(vk::CommandBuffer commandBuffer)
{
   updateUniformBuffers();

   const Mesh* mesh = resourceManager.getMesh(meshHandle);
   ASSERT(mesh);

   simpleRenderPass->render(commandBuffer, *mesh);
}

void Renderer::updateUniformBuffers()
{
   glm::mat4 worldToView = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();
   glm::mat4 viewToClip = glm::perspective(glm::radians(70.0f), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, 0.1f, 10.0f);
   viewToClip[1][1] *= -1.0f;

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;

   viewUniformBuffer->update(viewUniformData);
}

void Renderer::onSwapchainRecreated()
{
   colorTexture = createColorTexture(context);
   depthTexture = createDepthTexture(context);

   simpleRenderPass->onSwapchainRecreated(*colorTexture, *depthTexture);
}
