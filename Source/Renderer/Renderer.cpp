#include "Renderer/Renderer.h"

#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

#include "Renderer/Passes/Depth/DepthPass.h"
#include "Renderer/Passes/Forward/ForwardPass.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <span>

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

   bool outside(std::span<glm::vec3> points, const glm::vec4& plane)
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

   bool frustumCull(const glm::vec3& position, float radius, const std::array<glm::vec4, 6>& frustumPlanes)
   {
      for (const glm::vec4& plane : frustumPlanes)
      {
         if (signedPlaneDist(position, plane) < -radius)
         {
            return true;
         }
      }

      return false;
   }

   bool frustumCull(std::span<glm::vec3> points, const std::array<glm::vec4, 6>& frustumPlanes)
   {
      for (const glm::vec4& plane : frustumPlanes)
      {
         if (outside(points, plane))
         {
            return true;
         }
      }

      return false;
   }

   bool frustumCull(const Bounds& bounds, const std::array<glm::vec4, 6>& frustumPlanes)
   {
      // First check the bounding sphere
      if (frustumCull(bounds.getCenter(), bounds.getRadius(), frustumPlanes))
      {
         return true;
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
      if (frustumCull(corners, frustumPlanes))
      {
         return true;
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
            info.materials.resize(mesh->getNumSections());

            for (uint32_t section = 0; section < mesh->getNumSections(); ++section)
            {
               const MeshSection& meshSection = mesh->getSection(section);

               if (const Material* material = resourceManager.getMaterial(meshSection.materialHandle))
               {
                  bool visible = !frustumCull(transformBounds(meshSection.bounds, info.transform), frustumPlanes);
                  if (visible)
                  {
                     anyVisible = true;

                     BlendMode blendMode = material->getBlendMode();
                     if (blendMode == BlendMode::Opaque)
                     {
                        info.visibleOpaqueSections.push_back(section);
                     }
                     else if (blendMode == BlendMode::Translucent)
                     {
                        info.visibleTranslucentSections.push_back(section);
                     }

                     info.materials[section] = material;
                  }
               }
            }

            if (anyVisible)
            {
               sceneRenderInfo.meshes.push_back(info);
            }
         }
      });

      std::sort(sceneRenderInfo.meshes.begin(), sceneRenderInfo.meshes.end(), [viewPosition = view.getViewPosition()](const MeshRenderInfo& first, const MeshRenderInfo& second)
      {
         float firstSquaredDistance = glm::distance2(first.transform.position, viewPosition);
         float secondSquaredDistance = glm::distance2(second.transform.position, viewPosition);

         return firstSquaredDistance > secondSquaredDistance;
      });

      scene.forEach<TransformComponent, PointLightComponent>([&sceneRenderInfo, &frustumPlanes](const TransformComponent& transformComponent, const PointLightComponent& pointLightComponent)
      {
         PointLightRenderInfo info;
         info.color = pointLightComponent.getColor();
         info.position = transformComponent.getAbsoluteTransform().position;
         info.radius = pointLightComponent.getRadius();

         if (info.radius > 0.0f && glm::length2(info.color) > 0.0f)
         {
            bool visible = !frustumCull(info.position, info.radius, frustumPlanes);
            if (visible)
            {
               sceneRenderInfo.pointLights.push_back(info);
            }
         }
      });

      scene.forEach<TransformComponent, SpotLightComponent>([&sceneRenderInfo, &frustumPlanes](const TransformComponent& transformComponent, const SpotLightComponent& spotLightComponent)
      {
         Transform transform = transformComponent.getAbsoluteTransform();

         SpotLightRenderInfo info;
         info.color = spotLightComponent.getColor();
         info.position = transform.position;
         info.direction = transform.getForwardVector();
         info.radius = spotLightComponent.getRadius();
         info.beamAngle = glm::radians(spotLightComponent.getBeamAngle());
         info.cutoffAngle = glm::radians(spotLightComponent.getCutoffAngle());

         if (info.radius > 0.0f && glm::length2(info.color) > 0.0f)
         {
            glm::vec3 end = info.position + info.direction * info.radius;
            float endWidth = glm::tan(info.cutoffAngle) * info.radius;

            std::array<glm::vec3, 5> points =
            {
               info.position, // Origin of the light
               end + transform.getUpVector() * endWidth, // End top
               end - transform.getUpVector() * endWidth, // End bottom
               end + transform.getRightVector() * endWidth, // End right
               end - transform.getRightVector() * endWidth, // End left
            };

            bool visible = !frustumCull(points, frustumPlanes);
            if (visible)
            {
               sceneRenderInfo.spotLights.push_back(info);
            }
         }
      });

      scene.forEach<TransformComponent, DirectionalLightComponent>([&sceneRenderInfo, &frustumPlanes](const TransformComponent& transformComponent, const DirectionalLightComponent& directionalLightComponent)
      {
         DirectionalLightRenderInfo info;
         info.color = directionalLightComponent.getColor();
         info.direction = transformComponent.getAbsoluteTransform().getForwardVector();

         if (glm::length2(info.color) > 0.0f)
         {
            sceneRenderInfo.directionalLights.push_back(info);
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
      static const uint32_t kMaxUniformBuffers = 2;
      static const uint32_t kMaxSets = 2; // TODO

      vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
         .setType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(kMaxUniformBuffers * GraphicsContext::kMaxFramesInFlight);

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
      depthPass = std::make_unique<DepthPass>(context, resourceManager);
      forwardPass = std::make_unique<ForwardPass>(context, descriptorPool, resourceManager);
   }

   onSwapchainRecreated();
}

Renderer::~Renderer()
{
   forwardPass = nullptr;
   depthPass = nullptr;

   depthTexture = nullptr;
   colorTexture = nullptr;

   view = nullptr;

   device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void Renderer::render(vk::CommandBuffer commandBuffer, const Scene& scene)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();

   vk::Viewport viewport = vk::Viewport()
      .setX(0.0f)
      .setY(0.0f)
      .setWidth(static_cast<float>(swapchainExtent.width))
      .setHeight(static_cast<float>(swapchainExtent.height))
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);
   commandBuffer.setViewport(0, viewport);

   vk::Rect2D scissor = vk::Rect2D()
      .setOffset(vk::Offset2D(0, 0))
      .setExtent(swapchainExtent);
   commandBuffer.setScissor(0, scissor);

   view->update(scene);
   SceneRenderInfo sceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *view);

   depthPass->render(commandBuffer, sceneRenderInfo);
   forwardPass->render(commandBuffer, sceneRenderInfo);
}

void Renderer::onSwapchainRecreated()
{
   colorTexture = createColorTexture(context);
   depthTexture = createDepthTexture(context);

   updateRenderPassAttachments();
}

void Renderer::updateRenderPassAttachments()
{
   {
      RenderPassAttachments depthPassAttachments;
      depthPassAttachments.depthInfo = depthTexture->getInfo();

      depthPass->updateAttachments(depthPassAttachments);
   }

   {
      std::vector<TextureInfo> colorInfo;
      std::vector<TextureInfo> resolveInfo;
      if (colorTexture->getInfo().sampleCount == vk::SampleCountFlagBits::e1)
      {
         colorInfo.push_back(context.getSwapchain().getTextureInfo());
      }
      else
      {
         colorInfo.push_back(colorTexture->getInfo());
         resolveInfo.push_back(context.getSwapchain().getTextureInfo());
      }

      RenderPassAttachments forwardPassAttachments;
      forwardPassAttachments.depthInfo = depthTexture->getInfo();
      forwardPassAttachments.colorInfo = colorInfo;
      forwardPassAttachments.resolveInfo = resolveInfo;

      forwardPass->updateAttachments(forwardPassAttachments);
   }
}
