#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class NormalPass;
class NormalShader;
class ResourceManager;
class Texture;

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

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture);

protected:
   friend class SceneRenderPass<NormalPass>;

   bool supportsMaterialType(uint32_t typeMask) const;

   void renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;

   PipelineDescription<NormalPass> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const;
   Pipeline createPipeline(const PipelineDescription<NormalPass>& description, const AttachmentFormats& attachmentFormats);

private:
   NormalShader* normalShader = nullptr;

   vk::PipelineLayout pipelineLayout;
};
