#include "Renderer/Renderer.h"

#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

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

   std::array<glm::vec4, 6> computeFrustumPlanes(const glm::mat4& worldToClip)
   {
      std::array<glm::vec4, 6> frustumPlanes;

      for (int i = 0; i < frustumPlanes.size(); ++i)
      {
         glm::vec4& frustumPlane = frustumPlanes[i];

         int row = i / 2;
         int sign = (i % 2) == 0 ? 1 : -1;

         frustumPlane.x = worldToClip[0][3] + sign * worldToClip[0][row];
         frustumPlane.y = worldToClip[1][3] + sign * worldToClip[1][row];
         frustumPlane.z = worldToClip[2][3] + sign * worldToClip[2][row];
         frustumPlane.w = worldToClip[3][3] + sign * worldToClip[3][row];

         frustumPlane /= glm::length(glm::vec3(frustumPlane.x, frustumPlane.y, frustumPlane.z));
      }

      return frustumPlanes;
   }

   float signedPlaneDist(const glm::vec3& point, const glm::vec4& plane)
   {
      return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
   }

   bool outside(const std::array<glm::vec3, 8>& points, const glm::vec4& plane)
   {
      for (const glm::vec3& point : points)
      {
         if (signedPlaneDist(point, plane) >= 0.0f)
         {
            return false;
         }
      }

      return true;
   }

   bool frustumCull(const Bounds& bounds, const std::array<glm::vec4, 6>& frustumPlanes)
   {
      // First check if the bounding sphere is completely outside of any of the planes
      for (const glm::vec4& plane : frustumPlanes)
      {
         if (signedPlaneDist(bounds.getCenter(), plane) < -bounds.getRadius())
         {
            return true;
         }
      }

      // Next, check the bounding box
      glm::vec3 min = bounds.getMin();
      glm::vec3 max = bounds.getMax();
      std::array<glm::vec3, 8> corners =
      {
         glm::vec3(min.x, min.y, min.z),
         glm::vec3(min.x, min.y, max.z),
         glm::vec3(min.x, max.y, min.z),
         glm::vec3(min.x, max.y, max.z),
         glm::vec3(max.x, min.y, min.z),
         glm::vec3(max.x, min.y, max.z),
         glm::vec3(max.x, max.y, min.z),
         glm::vec3(max.x, max.y, max.z)
      };
      for (const glm::vec4& plane : frustumPlanes)
      {
         if (outside(corners, plane))
         {
            return true;
         }
      }

      return false;
   }

   Bounds transformBounds(const Bounds& bounds, const Transform& transform)
   {
      return Bounds(transform.transformPosition(bounds.getCenter()), transform.transformVector(bounds.getExtent()));
   }

   SceneRenderInfo computeSceneRenderInfo(const ResourceManager& resourceManager, const Scene& scene, const View& view)
   {
      SceneRenderInfo sceneRenderInfo(view);

      std::array<glm::vec4, 6> frustumPlanes = computeFrustumPlanes(view.getWorldToClip());

      scene.forEach<TransformComponent, MeshComponent>([&resourceManager, &sceneRenderInfo, &frustumPlanes](const TransformComponent& transformComponent, const MeshComponent& meshComponent)
      {
         if (const Mesh* mesh = resourceManager.getMesh(meshComponent.meshHandle))
         {
            MeshRenderInfo info(*mesh, transformComponent.getAbsoluteTransform());

            bool anyVisible = false;
            info.visibilityMask.resize(mesh->getNumSections());
            info.materials.resize(mesh->getNumSections());

            for (uint32_t section = 0; section < mesh->getNumSections(); ++section)
            {
               const MeshSection& meshSection = mesh->getSection(section);

               bool visible = !frustumCull(transformBounds(meshSection.bounds, info.transform), frustumPlanes);
               anyVisible |= visible;

               info.visibilityMask[section] = visible;
               info.materials[section] = resourceManager.getMaterial(meshSection.materialHandle);
            }

            if (anyVisible)
            {
               sceneRenderInfo.meshes.push_back(info);
            }
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
   view->update(scene);
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
