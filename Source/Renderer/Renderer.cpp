#include "Renderer/Renderer.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/Passes/Composite/CompositePass.h"
#include "Renderer/Passes/Depth/DepthPass.h"
#include "Renderer/Passes/Forward/ForwardPass.h"
#include "Renderer/Passes/Normal/NormalPass.h"
#include "Renderer/Passes/PostProcess/Bloom/BloomPass.h"
#include "Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"
#include "Renderer/Passes/SSAO/SSAOPass.h"
#include "Renderer/Passes/UI/UIPass.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

#include "Resources/ResourceManager.h"

#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/SkyboxComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/Systems/CameraSystem.h"

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

      if (const CameraSystem* cameraSystem = scene.getSystem<CameraSystem>())
      {
         if (Entity cameraEntity = cameraSystem->getActiveCamera())
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

   std::unique_ptr<Texture> createColorTexture(const GraphicsContext& context, vk::Format format, bool canBeSampled, vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1, uint32_t downscalingFactor = 1)
   {
      ASSERT(downscalingFactor > 0);

      const Swapchain& swapchain = context.getSwapchain();

      ImageProperties colorImageProperties;
      colorImageProperties.format = format;
      colorImageProperties.width = swapchain.getExtent().width / downscalingFactor;
      colorImageProperties.height = swapchain.getExtent().height / downscalingFactor;

      TextureProperties colorTextureProperties;
      colorTextureProperties.sampleCount = sampleCount;
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

   std::unique_ptr<Texture> createDepthTexture(const GraphicsContext& context, vk::Format format, vk::Extent2D extent, bool sampled, vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1)
   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = format;
      depthImageProperties.width = extent.width;
      depthImageProperties.height = extent.height;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = sampleCount;
      depthTextureProperties.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
      if (sampled)
      {
         depthTextureProperties.usage |= vk::ImageUsageFlagBits::eSampled;
      }
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

   SceneRenderInfo computeSceneRenderInfo(const ResourceManager& resourceManager, const Scene& scene, const View& view, bool isShadowPass)
   {
      SceneRenderInfo sceneRenderInfo(view);

      std::array<glm::vec4, 6> frustumPlanes = computeFrustumPlanes(view.getMatrices().worldToClip);

      scene.forEach<TransformComponent, MeshComponent>([&resourceManager, &sceneRenderInfo, &frustumPlanes, isShadowPass](const TransformComponent& transformComponent, const MeshComponent& meshComponent)
      {
         if (isShadowPass && !meshComponent.castsShadows)
         {
            return;
         }

         if (const Mesh* mesh = resourceManager.getMesh(meshComponent.meshHandle))
         {
            MeshRenderInfo info(*mesh, transformComponent.getAbsoluteTransform());

            bool anyVisible = false;
            uint32_t numSections = mesh->getNumSections();

            info.materials.resize(numSections);
            info.visibleOpaqueSections.reserve(numSections);
            info.visibleMaskedSections.reserve(numSections);
            info.visibleTranslucentSections.reserve(numSections);

            for (uint32_t section = 0; section < numSections; ++section)
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
               sceneRenderInfo.meshes.push_back(std::move(info));
            }
         }
      });

      std::sort(sceneRenderInfo.meshes.begin(), sceneRenderInfo.meshes.end(), [viewPosition = view.getMatrices().viewPosition](const MeshRenderInfo& first, const MeshRenderInfo& second)
      {
         float firstSquaredDistance = glm::distance2(first.transform.position, viewPosition);
         float secondSquaredDistance = glm::distance2(second.transform.position, viewPosition);

         return firstSquaredDistance > secondSquaredDistance;
      });

      if (!isShadowPass)
      {
         uint32_t allocatedPointShadowMaps = 0;
         scene.forEach<TransformComponent, PointLightComponent>([&sceneRenderInfo, &frustumPlanes, &allocatedPointShadowMaps](const TransformComponent& transformComponent, const PointLightComponent& pointLightComponent)
         {
            Transform transform = transformComponent.getAbsoluteTransform();

            PointLightRenderInfo info;
            info.color = pointLightComponent.getColor() * pointLightComponent.getBrightness();
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
            info.color = spotLightComponent.getColor() * spotLightComponent.getBrightness();
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
            info.color = directionalLightComponent.getColor() * directionalLightComponent.getBrightness();
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

Renderer::Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef, const RenderSettings& settings)
   : GraphicsResource(graphicsContext)
   , resourceManager(resourceManagerRef)
   , renderSettings(settings)
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
         NAME_POINTER(device, pointShadowViews[i], "Point Shadow View " + DebugUtils::toString(i));
      }
      for (uint32_t i = 0; i < spotShadowViews.size(); ++i)
      {
         spotShadowViews[i] = std::make_unique<View>(context, dynamicDescriptorPool);
         NAME_POINTER(device, spotShadowViews[i], "Spot Shadow View " + DebugUtils::toString(i));
      }
      for (uint32_t i = 0; i < directionalShadowViews.size(); ++i)
      {
         directionalShadowViews[i] = std::make_unique<View>(context, dynamicDescriptorPool);
         NAME_POINTER(device, directionalShadowViews[i], "Directional Shadow View " + DebugUtils::toString(i));
      }
   }

   {
      forwardLighting = std::make_unique<ForwardLighting>(context, dynamicDescriptorPool, depthStencilFormat);
      NAME_POINTER(device, forwardLighting, "Forward Lighting");
   }

   {
      normalPass = std::make_unique<NormalPass>(context, resourceManager);
      NAME_POINTER(device, normalPass, "Normal Pass");

      ssaoPass = std::make_unique<SSAOPass>(context, dynamicDescriptorPool, resourceManager);
      NAME_POINTER(device, ssaoPass, "SSAO Pass");

      shadowPass = std::make_unique<DepthPass>(context, resourceManager, true);
      NAME_POINTER(device, shadowPass, "Shadow Pass");

      forwardPass = std::make_unique<ForwardPass>(context, dynamicDescriptorPool, resourceManager, forwardLighting.get());
      NAME_POINTER(device, forwardPass, "Forward Pass");

      bloomPass = std::make_unique<BloomPass>(context, dynamicDescriptorPool, resourceManager);
      NAME_POINTER(device, bloomPass, "Bloom Pass");

      uiPass = std::make_unique<UIPass>(context);
      NAME_POINTER(device, uiPass, "UI Pass");

      compositePass = std::make_unique<CompositePass>(context, dynamicDescriptorPool, resourceManager);
      NAME_POINTER(device, compositePass, "Composite Pass");

      tonemapPass = std::make_unique<TonemapPass>(context, dynamicDescriptorPool, resourceManager);
      NAME_POINTER(device, tonemapPass, "Tonemap Pass");
   }

   onSwapchainRecreated();
}

Renderer::~Renderer()
{
   normalPass = nullptr;
   ssaoPass = nullptr;
   shadowPass = nullptr;
   forwardPass = nullptr;
   bloomPass = nullptr;
   uiPass = nullptr;
   compositePass = nullptr;
   tonemapPass = nullptr;

   depthTexture = nullptr;
   normalTexture = nullptr;
   ssaoTexture = nullptr;
   ssaoBlurTexture = nullptr;
   hdrColorTexture = nullptr;
   hdrResolveTexture = nullptr;
   roughnessMetalnessTexture = nullptr;
   uiColorTexture = nullptr;

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

   FrameAllocatorBase::reset();

   Texture* defaultBlackTexture = resourceManager.getDefaultTexture(DefaultTextureType::Black);
   Texture* defaultWhiteTexture = resourceManager.getDefaultTexture(DefaultTextureType::White);
   ASSERT(defaultBlackTexture && defaultWhiteTexture);

   INLINE_LABEL("Update main view");
   ViewInfo activeCameraViewInfo = computeActiveCameraViewInfo(context, scene);
   view->update(activeCameraViewInfo);

   SceneRenderInfo sceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *view, false);

   normalPass->render(commandBuffer, sceneRenderInfo, *depthTexture, *normalTexture);

   bool ssaoEnabled = renderSettings.ssaoQuality != RenderQuality::Disabled;
   if (ssaoEnabled)
   {
      ssaoPass->render(commandBuffer, sceneRenderInfo, *depthTexture, *normalTexture, *ssaoTexture, *ssaoBlurTexture, renderSettings.ssaoQuality);
   }

   renderShadowMaps(commandBuffer, scene, sceneRenderInfo);

   INLINE_LABEL("Update lighting");
   forwardLighting->update(sceneRenderInfo);

   const Texture* skyboxTexture = nullptr;
   scene.forEach<SkyboxComponent>([this, &skyboxTexture](const SkyboxComponent& skyboxComponent)
   {
      skyboxTexture = resourceManager.getTexture(skyboxComponent.textureHandle);
   });

   forwardPass->render(commandBuffer, sceneRenderInfo, *depthTexture, *hdrColorTexture, hdrResolveTexture.get(), *roughnessMetalnessTexture, *normalTexture, ssaoEnabled ? *ssaoTexture : *defaultWhiteTexture, skyboxTexture);

   bool bloomEnabled = renderSettings.bloomQuality != RenderQuality::Disabled;
   if (bloomEnabled)
   {
      bloomPass->render(commandBuffer, *hdrColorTexture, *defaultBlackTexture, renderSettings.bloomQuality);
   }

   uiPass->render(commandBuffer, *uiColorTexture);

   Texture& currentSwapchainTexture = context.getSwapchain().getCurrentTexture();
   tonemapPass->render(commandBuffer, currentSwapchainTexture, hdrResolveTexture ? *hdrResolveTexture : *hdrColorTexture, bloomEnabled ? bloomPass->getOutputTexture() : nullptr, uiColorTexture.get(), renderSettings.tonemapSettings);

   currentSwapchainTexture.transitionLayout(commandBuffer, TextureLayoutType::Present);
}

void Renderer::onSwapchainRecreated()
{
   bool msaaEnabled = renderSettings.msaaSamples != vk::SampleCountFlagBits::e1;

   std::array<vk::Format, 2> hdrColorFormats = { vk::Format::eB10G11R11UfloatPack32, vk::Format::eR16G16B16A16Sfloat };
   vk::Format hdrColorFormat = Texture::findSupportedFormat(context, hdrColorFormats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eColorAttachment);

   depthTexture = createDepthTexture(context, depthStencilFormat, context.getSwapchain().getExtent(), true, renderSettings.msaaSamples);
   normalTexture = createColorTexture(context, vk::Format::eR16G16B16A16Snorm, true, renderSettings.msaaSamples);
   ssaoTexture = createColorTexture(context, vk::Format::eR8Unorm, true);
   ssaoBlurTexture = createColorTexture(context, vk::Format::eR8Unorm, true);
   hdrColorTexture = createColorTexture(context, hdrColorFormat, !msaaEnabled, renderSettings.msaaSamples);
   if (msaaEnabled)
   {
      hdrResolveTexture = createColorTexture(context, hdrColorFormat, true);
   }
   else
   {
      hdrResolveTexture = nullptr;
   }
   roughnessMetalnessTexture = createColorTexture(context, vk::Format::eR8G8Unorm, true);
   uiColorTexture = createColorTexture(context, vk::Format::eR8G8B8A8Unorm, true);

   NAME_POINTER(device, depthTexture, "Depth Texture");
   NAME_POINTER(device, normalTexture, "Normal Texture");
   NAME_POINTER(device, ssaoTexture, "SSAO Texture");
   NAME_POINTER(device, ssaoBlurTexture, "SSAO Blur Texture");
   NAME_POINTER(device, hdrColorTexture, "HDR Color Texture");
   NAME_POINTER(device, hdrResolveTexture, "HDR Resolve Texture");
   NAME_POINTER(device, roughnessMetalnessTexture, "Roughness Metalness Texture");
   NAME_POINTER(device, uiColorTexture, "UI Color Texture");

   bloomPass->recreateTextures(hdrColorTexture->getImageProperties().format, hdrColorTexture->getTextureProperties().sampleCount);
   uiPass->onOutputTextureCreated(*uiColorTexture);
}

void Renderer::updateRenderSettings(const RenderSettings& settings)
{
   bool msaaSamplesChanged = renderSettings.msaaSamples != settings.msaaSamples;
   renderSettings = settings;

   if (msaaSamplesChanged)
   {
      onSwapchainRecreated();
   }
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
               INLINE_LABEL("Update point shadow view " + DebugUtils::toString(viewIndex));
               std::unique_ptr<View>& pointShadowView = pointShadowViews[viewIndex];
               pointShadowView->update(pointLightViewInfo);
               SceneRenderInfo shadowSceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *pointShadowView, true);

               shadowPass->render(commandBuffer, shadowSceneRenderInfo, forwardLighting->getPointShadowTextureArray(), forwardLighting->getPointShadowView(shadowMapIndex, faceIndex));
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

            INLINE_LABEL("Update spot shadow view " + DebugUtils::toString(shadowMapIndex));
            std::unique_ptr<View>& spotShadowView = spotShadowViews[shadowMapIndex];
            spotShadowView->update(spotLightInfo.shadowViewInfo.value());
            SceneRenderInfo shadowSceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *spotShadowView, true);

            shadowPass->render(commandBuffer, shadowSceneRenderInfo, forwardLighting->getSpotShadowTextureArray(), forwardLighting->getSpotShadowView(shadowMapIndex));
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

            INLINE_LABEL("Update directional shadow view " + DebugUtils::toString(shadowMapIndex));
            std::unique_ptr<View>& directionalShadowView = directionalShadowViews[shadowMapIndex];
            directionalShadowView->update(directionalLightInfo.shadowViewInfo.value());
            SceneRenderInfo shadowSceneRenderInfo = computeSceneRenderInfo(resourceManager, scene, *directionalShadowView, true);

            shadowPass->render(commandBuffer, shadowSceneRenderInfo, forwardLighting->getDirectionalShadowTextureArray(), forwardLighting->getDirectionalShadowView(shadowMapIndex));
         }
      }
   }

   forwardLighting->transitionShadowMapLayout(commandBuffer, true);
}
