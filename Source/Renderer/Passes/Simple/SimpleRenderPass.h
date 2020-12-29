#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"
#include "Graphics/UniformData.h"

#include <memory>
#include <vector>

class Mesh;
class ResourceManager;
class SimpleShader;
class Swapchain;
class Texture;

class SimpleRenderPass : public GraphicsResource
{
public:
   SimpleRenderPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture);

   ~SimpleRenderPass();

   void render(vk::CommandBuffer commandBuffer, const Mesh& mesh);

   void onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture);

   bool areDescriptorSetsAllocated() const;
   void allocateDescriptorSets(vk::DescriptorPool descriptorPool);
   void clearDescriptorSets();
   void updateDescriptorSets(const UniformBuffer<ViewUniformData>& viewUniformBuffer, const UniformBuffer<MeshUniformData>& meshUniformBuffer, const Texture& texture);

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
