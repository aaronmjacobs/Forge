#pragma once

#include "Graphics/GraphicsResource.h"

#include "Renderer/Passes/Forward/ForwardLighting.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Mesh;
class ResourceManager;
class ForwardShader;
class Texture;
class View;
struct SceneRenderInfo;

class ForwardRenderPass : public GraphicsResource
{
public:
   ForwardRenderPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture);
   ~ForwardRenderPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

   void onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture);

private:
   void initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture);
   void terminateSwapchainDependentResources();

   std::unique_ptr<ForwardShader> forwardShader;
   ForwardLighting lighting;

   vk::RenderPass renderPass;

   vk::PipelineLayout pipelineLayout;
   vk::Pipeline pipelineWithTextures;
   vk::Pipeline pipelineWithoutTextures;

   std::vector<vk::Framebuffer> framebuffers;
};
