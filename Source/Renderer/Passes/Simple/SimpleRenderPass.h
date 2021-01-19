#pragma once

#include "Graphics/GraphicsResource.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Mesh;
class ResourceManager;
class SimpleShader;
class Texture;
class View;
struct SceneRenderInfo;

class SimpleRenderPass : public GraphicsResource
{
public:
   SimpleRenderPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture);

   ~SimpleRenderPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

   void onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture);

private:
   void initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture);
   void terminateSwapchainDependentResources();

   std::unique_ptr<SimpleShader> simpleShader;

   vk::RenderPass renderPass;

   vk::PipelineLayout pipelineLayout;
   vk::Pipeline pipeline;

   std::vector<vk::Framebuffer> framebuffers;
};
