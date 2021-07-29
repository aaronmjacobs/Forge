#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/RenderPass.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/UniformData.h"

#include "Resources/ResourceManager.h"

#include <memory>
#include <vector>

class DepthPass;
class ForwardPass;
class Scene;
class SimpleRenderPass;
class Swapchain;
class Texture;
class TonemapPass;
class View;

class Renderer : public GraphicsResource
{
public:
   Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef);
   ~Renderer();

   void render(vk::CommandBuffer commandBuffer, const Scene& scene);

   void onSwapchainRecreated();
   void toggleMSAA();

private:
   void updateSwapchainDependentFramebuffers();

   ResourceManager& resourceManager;

   vk::Format depthStencilFormat = vk::Format::eUndefined;

   vk::DescriptorPool descriptorPool;

   std::unique_ptr<View> view;

   std::unique_ptr<Texture> depthTexture;
   std::unique_ptr<Texture> hdrColorTexture;
   std::unique_ptr<Texture> hdrResolveTexture;

   std::unique_ptr<DepthPass> prePass;
   std::unique_ptr<ForwardPass> forwardPass;
   std::unique_ptr<TonemapPass> tonemapPass;

   FramebufferHandle prePassFramebufferHandle;
   FramebufferHandle forwardPassFramebufferHandle;
   FramebufferHandle tonemapPassFramebufferHandle;

   bool enableMSAA = false;
};
