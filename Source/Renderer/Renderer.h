#pragma once

#include "Graphics/DynamicDescriptorPool.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/RenderPass.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/UniformData.h"
#include "Renderer/ViewInfo.h"

#include <memory>
#include <vector>

class CompositePass;
class DepthPass;
class ForwardLighting;
class ForwardPass;
class NormalPass;
class ResourceManager;
class Scene;
class SimpleRenderPass;
class Swapchain;
class Texture;
class TonemapPass;
class UIPass;
class View;
struct SceneRenderInfo;

class Renderer : public GraphicsResource
{
public:
   Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef);
   ~Renderer();

   void render(vk::CommandBuffer commandBuffer, const Scene& scene);

   void onSwapchainRecreated();
   void toggleMSAA();

private:
   void renderShadowMaps(vk::CommandBuffer commandBuffer, const Scene& scene, const SceneRenderInfo& sceneRenderInfo);

   void updateSwapchainDependentFramebuffers();

   ResourceManager& resourceManager;

   vk::Format depthStencilFormat = vk::Format::eUndefined;

   DynamicDescriptorPool dynamicDescriptorPool;

   std::unique_ptr<View> view;
   std::array<std::unique_ptr<View>, ForwardLighting::kMaxPointShadowMaps * kNumCubeFaces> pointShadowViews;
   std::array<std::unique_ptr<View>, ForwardLighting::kMaxSpotShadowMaps> spotShadowViews;
   std::array<std::unique_ptr<View>, ForwardLighting::kMaxDirectionalShadowMaps> directionalShadowViews;
   std::unique_ptr<ForwardLighting> forwardLighting;

   std::unique_ptr<Texture> depthTexture;
   std::unique_ptr<Texture> normalTexture;
   std::unique_ptr<Texture> hdrColorTexture;
   std::unique_ptr<Texture> hdrResolveTexture;
   std::unique_ptr<Texture> uiColorTexture;

   std::unique_ptr<NormalPass> normalPass;
   std::unique_ptr<DepthPass> shadowPass;
   std::unique_ptr<ForwardPass> forwardPass;
   std::unique_ptr<UIPass> uiPass;
   std::unique_ptr<CompositePass> compositePass;
   std::unique_ptr<TonemapPass> tonemapPass;

   FramebufferHandle normalPassFramebufferHandle;
   std::array<FramebufferHandle, ForwardLighting::kMaxSpotShadowMaps * kNumCubeFaces> pointShadowPassFramebufferHandles;
   std::array<FramebufferHandle, ForwardLighting::kMaxSpotShadowMaps> spotShadowPassFramebufferHandles;
   std::array<FramebufferHandle, ForwardLighting::kMaxDirectionalShadowMaps> directionalShadowPassFramebufferHandles;
   FramebufferHandle forwardPassFramebufferHandle;
   FramebufferHandle tonemapPassFramebufferHandle;
   FramebufferHandle uiPassFramebufferHandle;
   FramebufferHandle compositePassFramebufferHandle;

   bool enableMSAA = false;
};
