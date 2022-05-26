#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class ResourceManager;
class NormalPass;
class NormalShader;

template<>
struct PipelineDescription<NormalPass>
{
   bool withTextures = true;
   bool masked = false;
   bool twoSided = false;

   std::size_t hash() const
   {
      return (withTextures * 0b001) | (masked * 0b010) | (twoSided * 0b100);
   }

   bool operator==(const PipelineDescription<NormalPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<NormalPass>);

class NormalPass : public SceneRenderPass<NormalPass>
{
public:
   NormalPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
   ~NormalPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle);

protected:
   friend class SceneRenderPass<NormalPass>;

   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   void renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;

   PipelineDescription<NormalPass> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const;
   Pipeline createPipeline(const PipelineDescription<NormalPass>& description);

private:
   std::unique_ptr<NormalShader> normalShader;

   vk::PipelineLayout pipelineLayout;
};
