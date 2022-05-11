#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DepthMaskedShader;
class DepthPass;
class DepthShader;
class ResourceManager;
struct SceneRenderInfo;

template<>
struct PipelineDescription<DepthPass>
{
   bool masked = false;
   bool cubemap = false;

   std::size_t hash() const
   {
      return (masked * 0b01) | (cubemap * 0b10);
   }

   bool operator==(const PipelineDescription<DepthPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<DepthPass>);

class DepthPass : public SceneRenderPass<DepthPass>
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, bool shadowPass = false);
   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle);

protected:
   friend class SceneRenderPass<DepthPass>;

   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   void renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;

   PipelineDescription<DepthPass> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const;
   vk::Pipeline createPipeline(const PipelineDescription<DepthPass>& description);

private:
   std::unique_ptr<DepthShader> depthShader;
   std::unique_ptr<DepthMaskedShader> depthMaskedShader;
   bool isShadowPass = false;

   vk::PipelineLayout opaquePipelineLayout;
   vk::PipelineLayout maskedPipelineLayout;
};
