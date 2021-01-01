#pragma once

#include "Graphics/GraphicsResource.h"

#include <glm/glm.hpp>

#include <memory>

class DepthShader;
class Mesh;
class ResourceManager;
class Texture;
class View;

class DepthPass : public GraphicsResource
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& depthTexture);

   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const View& view, const Mesh& mesh, const glm::mat4& localToWorld);

   void onSwapchainRecreated(const Texture& depthTexture);

   void updateDescriptorSets(const View& view);

private:
   void initializeSwapchainDependentResources(const Texture& depthTexture);
   void terminateSwapchainDependentResources();

   std::unique_ptr<DepthShader> depthShader;

   vk::RenderPass renderPass;

   vk::PipelineLayout pipelineLayout;
   vk::Pipeline pipeline;

   vk::Framebuffer framebuffer;
};
