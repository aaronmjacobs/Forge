#pragma once

#include "Graphics/GraphicsResource.h"

#include "Renderer/SceneRenderInfo.h"

#include <memory>

class DepthShader;
class ResourceManager;
class Texture;
struct SceneRenderInfo;

class DepthPass : public GraphicsResource
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& depthTexture);

   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

   void onSwapchainRecreated(const Texture& depthTexture);

private:
   void initializeSwapchainDependentResources(const Texture& depthTexture);
   void terminateSwapchainDependentResources();

   std::unique_ptr<DepthShader> depthShader;

   vk::RenderPass renderPass;

   vk::PipelineLayout pipelineLayout;
   vk::Pipeline pipeline;

   vk::Framebuffer framebuffer;
};
