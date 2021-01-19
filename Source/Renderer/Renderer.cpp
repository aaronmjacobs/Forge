#include "Renderer/Renderer.h"

#include "Core/Math/MathUtils.h"

#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthPass.h"
#include "Renderer/Passes/Simple/SimpleRenderPass.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

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
      std::array<vk::Format, 5> depthFormats = { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm };
      depthImageProperties.format = Texture::findSupportedFormat(context, depthFormats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
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

   SceneRenderInfo computeSceneRenderInfo(const ResourceManager& resourceManager, const Scene& scene, const View& view)
   {
      SceneRenderInfo sceneRenderInfo(view);

      scene.forEach<TransformComponent, MeshComponent>([&resourceManager, &sceneRenderInfo](const TransformComponent& transformComponent, const MeshComponent& meshComponent)
      {
         if (const Mesh* mesh = resourceManager.getMesh(meshComponent.meshHandle))
         {
            MeshRenderInfo info(*mesh, transformComponent.getAbsoluteTransform());

            // TODO Visibility (frustum culling)

            info.materials.reserve(mesh->getNumSections());
            for (uint32_t section = 0; section < mesh->getNumSections(); ++section)
            {
               info.materials.push_back(resourceManager.getMaterial(mesh->getSection(section).materialHandle));
            }

            sceneRenderInfo.meshes.push_back(info);
         }
      });

      return sceneRenderInfo;
   }
}

Renderer::Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef)
   : GraphicsResource(graphicsContext)
   , resourceManager(resourceManagerRef)
{
   {
      static const uint32_t kMaxUniformBuffers = 1;
      static const uint32_t kMaxSets = 1; // TODO

      vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
         .setType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(kMaxUniformBuffers);

      vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
         .setPoolSizes(uniformPoolSize)
         .setMaxSets(kMaxSets * GraphicsContext::kMaxFramesInFlight);
      descriptorPool = device.createDescriptorPool(createInfo);
   }

   {
      view = std::make_unique<View>(context, descriptorPool);
      view->updateDescriptorSets();
   }

   {
      colorTexture = createColorTexture(context);
      depthTexture = createDepthTexture(context);
   }

   {
      depthPass = std::make_unique<DepthPass>(context, resourceManager, *depthTexture);
      simpleRenderPass = std::make_unique<SimpleRenderPass>(context, resourceManager, *colorTexture, *depthTexture);
   }
}

Renderer::~Renderer()
{
   simpleRenderPass = nullptr;
   depthPass = nullptr;

   depthTexture = nullptr;
   colorTexture = nullptr;

   view = nullptr;

   device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void Renderer::render(vk::CommandBuffer commandBuffer, const Scene& scene)
{
   view->update();
   SceneRenderInfo sceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *view);

   depthPass->render(commandBuffer, sceneRenderInfo);
   simpleRenderPass->render(commandBuffer, sceneRenderInfo);
}

void Renderer::onSwapchainRecreated()
{
   colorTexture = createColorTexture(context);
   depthTexture = createDepthTexture(context);

   depthPass->onSwapchainRecreated(*depthTexture);
   simpleRenderPass->onSwapchainRecreated(*colorTexture, *depthTexture);
}
