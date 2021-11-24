#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class ResourceManager;
class ForwardLighting;
class ForwardShader;
class SkyboxShader;

class ForwardPass : public SceneRenderPass<ForwardPass>
{
public:
   ForwardPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager, const ForwardLighting* forwardLighting);
   ~ForwardPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle, const Texture* skyboxTexture);

protected:
   friend class SceneRenderPass<ForwardPass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   void renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::Pipeline selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const;

private:
   std::unique_ptr<ForwardShader> forwardShader;
   std::unique_ptr<SkyboxShader> skyboxShader;
   const ForwardLighting* lighting = nullptr;
   DescriptorSet skyboxDescriptorSet;
   vk::Sampler skyboxSampler;
};
