#pragma once

#include "Graphics/DescriptorSet.h"

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class ResourceManager;
class Texture;
class TonemapShader;

class TonemapPass : public SceneRenderPass<TonemapPass>
{
public:
   TonemapPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager);
   ~TonemapPass();

   void render(vk::CommandBuffer commandBuffer, FramebufferHandle framebufferHandle, Texture& hdrColorTexture);

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

protected:
   friend class SceneRenderPass<TonemapPass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

private:
   std::unique_ptr<TonemapShader> tonemapShader;

   DescriptorSet descriptorSet;
   vk::Sampler sampler;
};
