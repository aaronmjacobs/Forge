#include "Renderer/Renderer.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

#include "Renderer/Passes/Depth/DepthPass.h"
#include "Renderer/Passes/Forward/ForwardPass.h"
#include "Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <span>

namespace
{
   ViewInfo computeActiveCameraViewInfo(const GraphicsContext& context, const Scene& scene)
   {
      ViewInfo viewInfo;

      vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();
      viewInfo.projectionMode = ProjectionMode::Perspective;
      viewInfo.perspectiveInfo.direction = PerspectiveDirection::FromTransform;
      viewInfo.perspectiveInfo.aspectRatio = static_cast<float>(swapchainExtent.width) / swapchainExtent.height;

      if (Entity cameraEntity = scene.getActiveCamera())
      {
         if (const TransformComponent* transformComponent = cameraEntity.tryGetComponent<TransformComponent>())
         {
            viewInfo.transform = transformComponent->getAbsoluteTransform();
         }

         if (const CameraComponent* cameraComponent = cameraEntity.tryGetComponent<CameraComponent>())
         {
            viewInfo.perspectiveInfo.fieldOfView = cameraComponent->fieldOfView;
            viewInfo.perspectiveInfo.nearPlane = cameraComponent->nearPlane;
            viewInfo.perspectiveInfo.farPlane = cameraComponent->farPlane;
         }
      }

      return viewInfo;
   }

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

   std::unique_ptr<Texture> createColorTexture(const GraphicsContext& context, vk::Format format, bool canBeSampled, bool enableMSAA)
   {
      const Swapchain& swapchain = context.getSwapchain();

      ImageProperties colorImageProperties;
      colorImageProperties.format = format;
      colorImageProperties.width = swapchain.getExtent().width;
      colorImageProperties.height = swapchain.getExtent().height;

      TextureProperties colorTextureProperties;
      colorTextureProperties.sampleCount = enableMSAA ? getMaxSampleCount(context) : vk::SampleCountFlagBits::e1;
      colorTextureProperties.usage = vk::ImageUsageFlagBits::eColorAttachment;
      if (canBeSampled)
      {
         colorTextureProperties.usage |= vk::ImageUsageFlagBits::eSampled;
      }
      else
      {
         colorTextureProperties.usage |= vk::ImageUsageFlagBits::eTransientAttachment;
      }
      colorTextureProperties.aspects = vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout colorInitialLayout;
      colorInitialLayout.layout = vk::ImageLayout::eColorAttachmentOptimal;
      colorInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

      return std::make_unique<Texture>(context, colorImageProperties, colorTextureProperties, colorInitialLayout);
   }

   std::unique_ptr<Texture> createDepthTexture(const GraphicsContext& context, vk::Format format, vk::Extent2D extent, bool enableMSAA)
   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = format;
      depthImageProperties.width = extent.width;
      depthImageProperties.height = extent.height;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = enableMSAA ? getMaxSampleCount(context) : vk::SampleCountFlagBits::e1;
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

   SceneRenderInfo computeSceneRenderInfo(const ResourceManager& resourceManager, const Scene& scene, const View& view, bool includeLights)
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

      if (includeLights)
      {
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
      }

      return sceneRenderInfo;
   }
}

Renderer::Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef)
   : GraphicsResource(graphicsContext)
   , resourceManager(resourceManagerRef)
{
   {
      std::array<vk::Format, 5> depthFormats = { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm };
      depthStencilFormat = Texture::findSupportedFormat(context, depthFormats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
   }

   {
      static const uint32_t kMaxUniformBuffers = 3;
      static const uint32_t kMaxImages = 1; // TODO
      static const uint32_t kMaxSets = 4; // TODO

      vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
         .setType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(kMaxUniformBuffers * GraphicsContext::kMaxFramesInFlight);

      vk::DescriptorPoolSize samplerPoolSize = vk::DescriptorPoolSize()
         .setType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(kMaxImages * GraphicsContext::kMaxFramesInFlight);

      std::array<vk::DescriptorPoolSize, 2> poolSizes = { uniformPoolSize, samplerPoolSize };

      vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
         .setPoolSizes(poolSizes)
         .setMaxSets(kMaxSets * GraphicsContext::kMaxFramesInFlight);
      descriptorPool = device.createDescriptorPool(createInfo);
   }

   {
      view = std::make_unique<View>(context, descriptorPool);
      NAME_POINTER(view, "Main View");
   }

   {
      prePass = std::make_unique<DepthPass>(context, resourceManager);
      NAME_POINTER(prePass, "Pre Pass");

      forwardPass = std::make_unique<ForwardPass>(context, descriptorPool, resourceManager);
      NAME_POINTER(forwardPass, "Forward Pass");

      tonemapPass = std::make_unique<TonemapPass>(context, descriptorPool, resourceManager);
      NAME_POINTER(tonemapPass, "Tonemap Pass");
   }

   onSwapchainRecreated();
}

Renderer::~Renderer()
{
   prePass = nullptr;
   forwardPass = nullptr;
   tonemapPass = nullptr;

   depthTexture = nullptr;
   hdrColorTexture = nullptr;
   hdrResolveTexture = nullptr;

   view = nullptr;

   device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void Renderer::render(vk::CommandBuffer commandBuffer, const Scene& scene)
{
   SCOPED_LABEL("Scene");

   INLINE_LABEL("Update main view");
   ViewInfo activeCameraViewInfo = computeActiveCameraViewInfo(context, scene);
   view->update(activeCameraViewInfo);

   SceneRenderInfo sceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *view, true);

   prePass->render(commandBuffer, sceneRenderInfo, prePassFramebufferHandle);
   forwardPass->render(commandBuffer, sceneRenderInfo, forwardPassFramebufferHandle);
   tonemapPass->render(commandBuffer, tonemapPassFramebufferHandle, hdrResolveTexture ? *hdrResolveTexture : *hdrColorTexture);
}

void Renderer::onSwapchainRecreated()
{
   depthTexture = createDepthTexture(context, depthStencilFormat, context.getSwapchain().getExtent(), enableMSAA);
   hdrColorTexture = createColorTexture(context, vk::Format::eR16G16B16A16Sfloat, !enableMSAA, enableMSAA);
   if (enableMSAA)
   {
      hdrResolveTexture = createColorTexture(context, vk::Format::eR16G16B16A16Sfloat, true, false);
   }
   else
   {
      hdrResolveTexture = nullptr;
   }

   NAME_POINTER(depthTexture, "Depth Texture");
   NAME_POINTER(hdrColorTexture, "HDR Color Texture");
   NAME_POINTER(hdrResolveTexture, "HDR Resolve Texture");

   updateSwapchainDependentFramebuffers();
}

void Renderer::toggleMSAA()
{
   enableMSAA = !enableMSAA;
   onSwapchainRecreated();
}

void Renderer::updateSwapchainDependentFramebuffers()
{
   {
      AttachmentInfo prePassInfo;
      prePassInfo.depthInfo = depthTexture->getInfo();

      prePass->updateAttachmentSetup(prePassInfo.asBasic());
      prePassFramebufferHandle = prePass->createFramebuffer(prePassInfo);
   }

   {
      AttachmentInfo forwardInfo;
      forwardInfo.depthInfo = depthTexture->getInfo();
      forwardInfo.colorInfo = { hdrColorTexture->getInfo() };
      if (hdrResolveTexture)
      {
         forwardInfo.resolveInfo = { hdrResolveTexture->getInfo() };
      }

      forwardPass->updateAttachmentSetup(forwardInfo.asBasic());
      forwardPassFramebufferHandle = forwardPass->createFramebuffer(forwardInfo);
   }

   {
      AttachmentInfo tonemapInfo;
      tonemapInfo.colorInfo = { context.getSwapchain().getTextureInfo() };

      tonemapPass->updateAttachmentSetup(tonemapInfo.asBasic());
      tonemapPassFramebufferHandle = tonemapPass->createFramebuffer(tonemapInfo);
   }
}
