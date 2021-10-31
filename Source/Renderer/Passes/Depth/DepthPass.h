#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DepthMaskedShader;
class DepthShader;
class ResourceManager;
struct SceneRenderInfo;

class DepthPass : public SceneRenderPass<DepthPass>
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, bool shadowPass = false);
   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle);

protected:
   friend class SceneRenderPass<DepthPass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   void renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;
   vk::Pipeline selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const;

private:
   std::unique_ptr<DepthShader> depthShader;
   std::unique_ptr<DepthMaskedShader> depthMaskedShader;
   bool isShadowPass = false;
};
