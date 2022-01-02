#pragma once

#include "Graphics/DescriptorSet.h"

#include "Renderer/Passes/Composite/CompositeShader.h"
#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DynamicDescriptorPool;
class ResourceManager;
class Texture;

class CompositePass : public SceneRenderPass<CompositePass>
{
public:
   CompositePass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~CompositePass();

   void render(vk::CommandBuffer commandBuffer, FramebufferHandle framebufferHandle, Texture& sourceTexture, CompositeShader::Mode mode);

protected:
   friend class SceneRenderPass<CompositePass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

private:
   std::unique_ptr<CompositeShader> compositeShader;

   DescriptorSet descriptorSet;
   vk::Sampler sampler;
};
