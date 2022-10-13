#pragma once

#include "Graphics/DynamicDescriptorPool.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/RenderPass.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/UniformData.h"
#include "Renderer/ViewInfo.h"

#include <memory>
#include <vector>

class BloomPass;
class CompositePass;
class DepthPass;
class ForwardLighting;
class ForwardPass;
class NormalPass;
class ResourceManager;
class Scene;
class SimpleRenderPass;
class SSAOPass;
class Swapchain;
class Texture;
class TonemapPass;
class UIPass;
class View;
struct SceneRenderInfo;

class Renderer : public GraphicsResource
{
public:
   Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef, const RenderSettings& settings);
   ~Renderer();

   void render(vk::CommandBuffer commandBuffer, const Scene& scene);

   void onSwapchainRecreated();
   void updateRenderSettings(const RenderSettings& settings);

private:
   void renderShadowMaps(vk::CommandBuffer commandBuffer, const Scene& scene, const SceneRenderInfo& sceneRenderInfo);

   ResourceManager& resourceManager;

   RenderSettings renderSettings;

   vk::Format depthStencilFormat = vk::Format::eUndefined;

   DynamicDescriptorPool dynamicDescriptorPool;

   std::unique_ptr<View> view;
   std::array<std::unique_ptr<View>, ForwardLighting::kMaxPointShadowMaps * kNumCubeFaces> pointShadowViews;
   std::array<std::unique_ptr<View>, ForwardLighting::kMaxSpotShadowMaps> spotShadowViews;
   std::array<std::unique_ptr<View>, ForwardLighting::kMaxDirectionalShadowMaps> directionalShadowViews;
   std::unique_ptr<ForwardLighting> forwardLighting;

   std::unique_ptr<Texture> defaultBlackTexture;
   std::unique_ptr<Texture> defaultWhiteTexture;
   std::unique_ptr<Texture> depthTexture;
   std::unique_ptr<Texture> normalTexture;
   std::unique_ptr<Texture> ssaoTexture;
   std::unique_ptr<Texture> ssaoBlurTexture;
   std::unique_ptr<Texture> hdrColorTexture;
   std::unique_ptr<Texture> hdrResolveTexture;
   std::unique_ptr<Texture> roughnessMetalnessTexture;
   std::unique_ptr<Texture> uiColorTexture;

   std::unique_ptr<NormalPass> normalPass;
   std::unique_ptr<SSAOPass> ssaoPass;
   std::unique_ptr<DepthPass> shadowPass;
   std::unique_ptr<ForwardPass> forwardPass;
   std::unique_ptr<BloomPass> bloomPass;
   std::unique_ptr<UIPass> uiPass;
   std::unique_ptr<CompositePass> compositePass;
   std::unique_ptr<TonemapPass> tonemapPass;
};
