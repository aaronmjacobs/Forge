#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/UniformData.h"

#include <memory>
#include <vector>

class Mesh;
class ResourceManager;
class SimpleShader;
class Swapchain;
class Texture;
class View;

class SimpleRenderPass : public GraphicsResource
{
public:
   SimpleRenderPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture);

   ~SimpleRenderPass();

   void render(vk::CommandBuffer commandBuffer, const View& view, const Mesh& mesh);

   void onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture);

   void updateDescriptorSets(const View& view, const Texture& texture);

private:
   void initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture);
   void terminateSwapchainDependentResources();

   std::unique_ptr<SimpleShader> simpleShader;
   vk::Sampler sampler;

   vk::RenderPass renderPass;

   vk::PipelineLayout pipelineLayout;
   vk::Pipeline pipeline;

   std::vector<vk::Framebuffer> framebuffers;
};
