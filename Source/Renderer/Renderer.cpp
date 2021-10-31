#include "Renderer/Renderer.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

#include "Renderer/ForwardLighting.h"
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

   ViewInfo computePointLightShadowViewInfo(const Transform& transform, const PointLightComponent& pointLightComponent)
   {
      ViewInfo viewInfo;

      viewInfo.transform = transform;

      viewInfo.projectionMode = ProjectionMode::Perspective;

      viewInfo.perspectiveInfo.fieldOfView = 90.0f;
      viewInfo.perspectiveInfo.aspectRatio = 1.0f;
      viewInfo.perspectiveInfo.nearPlane = pointLightComponent.getShadowNearPlane();
      viewInfo.perspectiveInfo.farPlane = glm::max(pointLightComponent.getShadowNearPlane(), pointLightComponent.getRadius());

      viewInfo.depthBiasConstantFactor = pointLightComponent.getShadowBiasConstantFactor();
      viewInfo.depthBiasSlopeFactor = pointLightComponent.getShadowBiasSlopeFactor();
      viewInfo.depthBiasClamp = pointLightComponent.getShadowBiasClamp();

      return viewInfo;
   }

   ViewInfo computeSpotLightShadowViewInfo(const Transform& transform, const SpotLightComponent& spotLightComponent)
   {
      ViewInfo viewInfo;

      viewInfo.transform = transform;

      viewInfo.projectionMode = ProjectionMode::Perspective;

      viewInfo.perspectiveInfo.fieldOfView = spotLightComponent.getCutoffAngle() * 2.0f;
      viewInfo.perspectiveInfo.aspectRatio = 1.0f;
      viewInfo.perspectiveInfo.nearPlane = spotLightComponent.getShadowNearPlane();
      viewInfo.perspectiveInfo.farPlane = glm::max(spotLightComponent.getShadowNearPlane(), spotLightComponent.getRadius());

      viewInfo.depthBiasConstantFactor = spotLightComponent.getShadowBiasConstantFactor();
      viewInfo.depthBiasSlopeFactor = spotLightComponent.getShadowBiasSlopeFactor();
      viewInfo.depthBiasClamp = spotLightComponent.getShadowBiasClamp();

      return viewInfo;
   }

   ViewInfo computeDirectionalLightShadowViewInfo(const Transform& transform, const DirectionalLightComponent& directionalLightComponent)
   {
      ViewInfo viewInfo;

      viewInfo.transform = transform;

      viewInfo.projectionMode = ProjectionMode::Orthographic;

      viewInfo.orthographicInfo.width = directionalLightComponent.getShadowWidth() * transform.scale.x;
      viewInfo.orthographicInfo.height = directionalLightComponent.getShadowHeight() * transform.scale.z;
      viewInfo.orthographicInfo.depth = directionalLightComponent.getShadowDepth() * transform.scale.y;

      viewInfo.depthBiasConstantFactor = directionalLightComponent.getShadowBiasConstantFactor();
      viewInfo.depthBiasSlopeFactor = directionalLightComponent.getShadowBiasSlopeFactor();
      viewInfo.depthBiasClamp = directionalLightComponent.getShadowBiasClamp();

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

      std::array<glm::vec4, 6> frustumPlanes = computeFrustumPlanes(view.getMatrices().worldToClip);

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
                     else if (blendMode == BlendMode::Masked)
                     {
                        info.visibleMaskedSections.push_back(section);
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

      std::sort(sceneRenderInfo.meshes.begin(), sceneRenderInfo.meshes.end(), [viewPosition = view.getMatrices().viewPosition](const MeshRenderInfo& first, const MeshRenderInfo& second)
      {
         float firstSquaredDistance = glm::distance2(first.transform.position, viewPosition);
         float secondSquaredDistance = glm::distance2(second.transform.position, viewPosition);

         return firstSquaredDistance > secondSquaredDistance;
      });

      if (includeLights)
      {
         uint32_t allocatedPointShadowMaps = 0;
         scene.forEach<TransformComponent, PointLightComponent>([&sceneRenderInfo, &frustumPlanes, &allocatedPointShadowMaps](const TransformComponent& transformComponent, const PointLightComponent& pointLightComponent)
         {
            Transform transform = transformComponent.getAbsoluteTransform();

            PointLightRenderInfo info;
            info.color = pointLightComponent.getColor();
            info.position = transform.position;
            info.radius = pointLightComponent.getRadius();
            info.shadowNearPlane = glm::min(pointLightComponent.getShadowNearPlane(), pointLightComponent.getRadius());

            if (info.radius > 0.0f && glm::length2(info.color) > 0.0f)
            {
               bool visible = !frustumCull(info.position, info.radius, frustumPlanes);
               if (visible)
               {
                  if (pointLightComponent.castsShadows() && allocatedPointShadowMaps < ForwardLighting::kMaxPointShadowMaps)
                  {
                     info.shadowViewInfo = computePointLightShadowViewInfo(transform, pointLightComponent);
                     info.shadowMapIndex = allocatedPointShadowMaps++;
                  }

                  sceneRenderInfo.pointLights.push_back(info);
               }
            }
         });

         uint32_t allocatedSpotShadowMaps = 0;
         scene.forEach<TransformComponent, SpotLightComponent>([&sceneRenderInfo, &frustumPlanes, &allocatedSpotShadowMaps](const TransformComponent& transformComponent, const SpotLightComponent& spotLightComponent)
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
                  if (spotLightComponent.castsShadows() && allocatedSpotShadowMaps < ForwardLighting::kMaxSpotShadowMaps)
                  {
                     info.shadowViewInfo = computeSpotLightShadowViewInfo(transform, spotLightComponent);
                     info.shadowMapIndex = allocatedSpotShadowMaps++;
                  }

                  sceneRenderInfo.spotLights.push_back(info);
               }
            }
         });

         uint32_t allocatedDirectionalShadowMaps = 0;
         scene.forEach<TransformComponent, DirectionalLightComponent>([&sceneRenderInfo, &frustumPlanes, &allocatedDirectionalShadowMaps](const TransformComponent& transformComponent, const DirectionalLightComponent& directionalLightComponent)
         {
            Transform transform = transformComponent.getAbsoluteTransform();

            DirectionalLightRenderInfo info;
            info.color = directionalLightComponent.getColor();
            info.direction = transform.getForwardVector();

            if (glm::length2(info.color) > 0.0f)
            {
               if (directionalLightComponent.castsShadows() && allocatedDirectionalShadowMaps < ForwardLighting::kMaxDirectionalShadowMaps)
               {
                  ViewInfo shadowViewInfo = computeDirectionalLightShadowViewInfo(transform, directionalLightComponent);
                  glm::vec3 forwardOffset = transform.getForwardVector() * shadowViewInfo.orthographicInfo.depth;
                  glm::vec3 rightOffset = transform.getRightVector() * shadowViewInfo.orthographicInfo.width;
                  glm::vec3 upOffset = transform.getUpVector() * shadowViewInfo.orthographicInfo.height;

                  std::array<glm::vec3, 8> points =
                  {
                     transform.position + forwardOffset + rightOffset + upOffset,
                     transform.position + forwardOffset + rightOffset - upOffset,
                     transform.position + forwardOffset - rightOffset + upOffset,
                     transform.position + forwardOffset - rightOffset - upOffset,
                     transform.position - forwardOffset + rightOffset + upOffset,
                     transform.position - forwardOffset + rightOffset - upOffset,
                     transform.position - forwardOffset - rightOffset + upOffset,
                     transform.position - forwardOffset - rightOffset - upOffset,
                  };

                  bool shadowsVisible = !frustumCull(points, frustumPlanes);
                  if (shadowsVisible)
                  {
                     info.shadowViewInfo = shadowViewInfo;
                     info.shadowMapIndex = allocatedDirectionalShadowMaps++;
                     info.shadowOrthoDepth = shadowViewInfo.orthographicInfo.depth;
                  }
               }

               sceneRenderInfo.directionalLights.push_back(info);
            }
         });
      }

      return sceneRenderInfo;
   }

   DynamicDescriptorPool::Sizes getDynamicDescriptorPoolSizes()
   {
      DynamicDescriptorPool::Sizes sizes;

      sizes.maxSets = 50;

      sizes.uniformBufferCount = 30;
      sizes.combinedImageSamplerCount = 10;

      return sizes;
   }
}

Renderer::Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef)
   : GraphicsResource(graphicsContext)
   , resourceManager(resourceManagerRef)
   , dynamicDescriptorPool(graphicsContext, getDynamicDescriptorPoolSizes())
{
   NAME_ITEM(context.getDevice(), dynamicDescriptorPool, "Renderer Dynamic Descriptor Pool");

   {
      std::array<vk::Format, 5> depthFormats = { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm };
      depthStencilFormat = Texture::findSupportedFormat(context, depthFormats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
   }

   {
      view = std::make_unique<View>(context, dynamicDescriptorPool);
      NAME_POINTER(device, view, "Main View");

      for (uint32_t i = 0; i < pointShadowViews.size(); ++i)
      {
         pointShadowViews[i] = std::make_unique<View>(context, dynamicDescriptorPool);
         NAME_POINTER(device, pointShadowViews[i], "Point Shadow View " + std::to_string(i));
      }
      for (uint32_t i = 0; i < spotShadowViews.size(); ++i)
      {
         spotShadowViews[i] = std::make_unique<View>(context, dynamicDescriptorPool);
         NAME_POINTER(device, spotShadowViews[i], "Spot Shadow View " + std::to_string(i));
      }
      for (uint32_t i = 0; i < directionalShadowViews.size(); ++i)
      {
         directionalShadowViews[i] = std::make_unique<View>(context, dynamicDescriptorPool);
         NAME_POINTER(device, directionalShadowViews[i], "Directional Shadow View " + std::to_string(i));
      }
   }

   {
      forwardLighting = std::make_unique<ForwardLighting>(context, dynamicDescriptorPool, depthStencilFormat);
      NAME_POINTER(device, forwardLighting, "Forward Lighting");
   }

   {
      prePass = std::make_unique<DepthPass>(context, resourceManager);
      NAME_POINTER(device, prePass, "Pre Pass");

      shadowPass = std::make_unique<DepthPass>(context, resourceManager, true);
      NAME_POINTER(device, shadowPass, "Shadow Pass");

      forwardPass = std::make_unique<ForwardPass>(context, resourceManager, forwardLighting.get());
      NAME_POINTER(device, forwardPass, "Forward Pass");

      tonemapPass = std::make_unique<TonemapPass>(context, dynamicDescriptorPool, resourceManager);
      NAME_POINTER(device, tonemapPass, "Tonemap Pass");
   }

   {
      BasicTextureInfo shadowPassDepthSetupInfo;
      shadowPassDepthSetupInfo.format = depthStencilFormat;
      shadowPassDepthSetupInfo.sampleCount = vk::SampleCountFlagBits::e1;
      shadowPassDepthSetupInfo.isSwapchainTexture = false;

      BasicAttachmentInfo shadowPassSetupInfo;
      shadowPassSetupInfo.depthInfo = shadowPassDepthSetupInfo;

      shadowPass->updateAttachmentSetup(shadowPassSetupInfo);

      for (uint32_t shadowMapIndex = 0; shadowMapIndex < ForwardLighting::kMaxPointShadowMaps; ++shadowMapIndex)
      {
         for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
         {
            uint32_t viewIndex = ForwardLighting::getPointViewIndex(shadowMapIndex, faceIndex);

            AttachmentInfo shadowPassInfo;
            shadowPassInfo.depthInfo = forwardLighting->getPointShadowInfo(shadowMapIndex, faceIndex);

            pointShadowPassFramebufferHandles[viewIndex] = shadowPass->createFramebuffer(shadowPassInfo);
         }
      }

      for (uint32_t i = 0; i < ForwardLighting::kMaxSpotShadowMaps; ++i)
      {
         AttachmentInfo shadowPassInfo;
         shadowPassInfo.depthInfo = forwardLighting->getSpotShadowInfo(i);

         spotShadowPassFramebufferHandles[i] = shadowPass->createFramebuffer(shadowPassInfo);
      }

      for (uint32_t i = 0; i < ForwardLighting::kMaxDirectionalShadowMaps; ++i)
      {
         AttachmentInfo shadowPassInfo;
         shadowPassInfo.depthInfo = forwardLighting->getDirectionalShadowInfo(i);

         directionalShadowPassFramebufferHandles[i] = shadowPass->createFramebuffer(shadowPassInfo);
      }
   }

   onSwapchainRecreated();
}

Renderer::~Renderer()
{
   prePass = nullptr;
   shadowPass = nullptr;
   forwardPass = nullptr;
   tonemapPass = nullptr;

   depthTexture = nullptr;
   hdrColorTexture = nullptr;
   hdrResolveTexture = nullptr;

   forwardLighting = nullptr;

   view = nullptr;
   for (std::unique_ptr<View>& pointShadowView : pointShadowViews)
   {
      pointShadowView.reset();
   }
   for (std::unique_ptr<View>& spotShadowView : spotShadowViews)
   {
      spotShadowView.reset();
   }
   for (std::unique_ptr<View>& directionalShadowView : directionalShadowViews)
   {
      directionalShadowView.reset();
   }
}

void Renderer::render(vk::CommandBuffer commandBuffer, const Scene& scene)
{
   SCOPED_LABEL("Scene");

   INLINE_LABEL("Update main view");
   ViewInfo activeCameraViewInfo = computeActiveCameraViewInfo(context, scene);
   view->update(activeCameraViewInfo);

   SceneRenderInfo sceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *view, true);

   prePass->render(commandBuffer, sceneRenderInfo, prePassFramebufferHandle);

   renderShadowMaps(commandBuffer, scene, sceneRenderInfo);

   INLINE_LABEL("Update lighting");
   forwardLighting->update(sceneRenderInfo);

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

   NAME_POINTER(device, depthTexture, "Depth Texture");
   NAME_POINTER(device, hdrColorTexture, "HDR Color Texture");
   NAME_POINTER(device, hdrResolveTexture, "HDR Resolve Texture");

   updateSwapchainDependentFramebuffers();
}

void Renderer::toggleMSAA()
{
   enableMSAA = !enableMSAA;
   onSwapchainRecreated();
}

void Renderer::renderShadowMaps(vk::CommandBuffer commandBuffer, const Scene& scene, const SceneRenderInfo& sceneRenderInfo)
{
   SCOPED_LABEL("Shadow maps");

   forwardLighting->transitionShadowMapLayout(commandBuffer, false);

   if (!sceneRenderInfo.pointLights.empty())
   {
      SCOPED_LABEL("Point lights");

      for (const PointLightRenderInfo& pointLightInfo : sceneRenderInfo.pointLights)
      {
         if (pointLightInfo.shadowViewInfo.has_value() && pointLightInfo.shadowMapIndex.has_value() && pointLightInfo.shadowMapIndex.value() < ForwardLighting::kMaxPointShadowMaps)
         {
            ViewInfo pointLightViewInfo = pointLightInfo.shadowViewInfo.value();
            uint32_t shadowMapIndex = pointLightInfo.shadowMapIndex.value();

            for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
            {
               CubeFace cubeFace = static_cast<CubeFace>(faceIndex);
               pointLightViewInfo.cubeFace = cubeFace;

               uint32_t viewIndex = ForwardLighting::getPointViewIndex(shadowMapIndex, faceIndex);
               INLINE_LABEL("Update point shadow view " + std::to_string(viewIndex));
               std::unique_ptr<View>& pointShadowView = pointShadowViews[viewIndex];
               pointShadowView->update(pointLightViewInfo);
               SceneRenderInfo shadowSceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *pointShadowView, false);

               shadowPass->render(commandBuffer, shadowSceneRenderInfo, pointShadowPassFramebufferHandles[viewIndex]);
            }
         }
      }
   }

   if (!sceneRenderInfo.spotLights.empty())
   {
      SCOPED_LABEL("Spot lights");

      for (const SpotLightRenderInfo& spotLightInfo : sceneRenderInfo.spotLights)
      {
         if (spotLightInfo.shadowViewInfo.has_value() && spotLightInfo.shadowMapIndex.has_value() && spotLightInfo.shadowMapIndex.value() < ForwardLighting::kMaxSpotShadowMaps)
         {
            uint32_t shadowMapIndex = spotLightInfo.shadowMapIndex.value();

            INLINE_LABEL("Update spot shadow view " + std::to_string(shadowMapIndex));
            std::unique_ptr<View>& spotShadowView = spotShadowViews[shadowMapIndex];
            spotShadowView->update(spotLightInfo.shadowViewInfo.value());
            SceneRenderInfo shadowSceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *spotShadowView, false);

            shadowPass->render(commandBuffer, shadowSceneRenderInfo, spotShadowPassFramebufferHandles[shadowMapIndex]);
         }
      }
   }

   if (!sceneRenderInfo.directionalLights.empty())
   {
      SCOPED_LABEL("Directional lights");

      for (const DirectionalLightRenderInfo& directionalLightInfo : sceneRenderInfo.directionalLights)
      {
         if (directionalLightInfo.shadowViewInfo.has_value() && directionalLightInfo.shadowMapIndex.has_value() && directionalLightInfo.shadowMapIndex.value() < ForwardLighting::kMaxDirectionalShadowMaps)
         {
            uint32_t shadowMapIndex = directionalLightInfo.shadowMapIndex.value();

            INLINE_LABEL("Update directional shadow view " + std::to_string(shadowMapIndex));
            std::unique_ptr<View>& directionalShadowView = directionalShadowViews[shadowMapIndex];
            directionalShadowView->update(directionalLightInfo.shadowViewInfo.value());
            SceneRenderInfo shadowSceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *directionalShadowView, false);

            shadowPass->render(commandBuffer, shadowSceneRenderInfo, directionalShadowPassFramebufferHandles[shadowMapIndex]);
         }
      }
   }

   forwardLighting->transitionShadowMapLayout(commandBuffer, true);
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
