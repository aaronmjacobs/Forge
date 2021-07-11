#pragma once

#include "Graphics/GraphicsResource.h"
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
class View;

class Renderer : public GraphicsResource
{
public:
   Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef);

   ~Renderer();

   void render(vk::CommandBuffer commandBuffer, const Scene& scene);

   void onSwapchainRecreated();

private:
   void updateRenderPassAttachments();

   ResourceManager& resourceManager;

   vk::DescriptorPool descriptorPool;

   std::unique_ptr<View> view;

   std::unique_ptr<Texture> colorTexture;
   std::unique_ptr<Texture> depthTexture;

   std::unique_ptr<DepthPass> depthPass;
   std::unique_ptr<ForwardPass> forwardPass;
};
