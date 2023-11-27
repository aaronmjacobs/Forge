#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DepthMaskedShader;
class DepthPass;
class DepthShader;
class ResourceManager;
class Texture;
struct SceneRenderInfo;

template<>
struct PipelineDescription<DepthPass>
{
   bool masked = false;
   bool twoSided = false;
   bool cubemap = false;

   std::size_t hash() const
   {
      return (masked * 0b001) | (twoSided * 0b010) | (cubemap * 0b100);
   }

   bool operator==(const PipelineDescription<DepthPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<DepthPass>);

class DepthPass : public SceneRenderPass<DepthPass>
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, bool shadowPass = false);
   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, vk::ImageView depthTextureView = nullptr);

protected:
   friend class SceneRenderPass<DepthPass>;

   bool supportsMaterialType(uint32_t typeMask) const;

   void renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;

   PipelineDescription<DepthPass> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const;
   Pipeline createPipeline(const PipelineDescription<DepthPass>& description, const AttachmentFormats& attachmentFormats);

private:
   DepthShader* depthShader = nullptr;
   DepthMaskedShader* depthMaskedShader = nullptr;
   bool isShadowPass = false;

   vk::PipelineLayout opaquePipelineLayout;
   vk::PipelineLayout maskedPipelineLayout;
};
