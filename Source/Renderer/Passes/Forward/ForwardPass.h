#pragma once

#include "Renderer/Passes/Forward/ForwardShader.h"
#include "Renderer/Passes/Forward/SkyboxShader.h"
#include "Renderer/Passes/SceneRenderPass.h"

#include "Graphics/DescriptorSet.h"

#include <memory>

class ResourceManager;
class ForwardLighting;
class ForwardPass;
class ForwardShader;
class SkyboxShader;

template<>
struct PipelineDescription<ForwardPass>
{
   bool withTextures = true;
   bool withBlending = false;
   bool twoSided = false;
   bool skybox = false;

   std::size_t hash() const
   {
      return (withTextures * 0b0001) | (withBlending * 0b0010) | (twoSided * 0b0100) | (skybox * 0b1000);
   }

   bool operator==(const PipelineDescription<ForwardPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<ForwardPass>);

class ForwardPass : public SceneRenderPass<ForwardPass>
{
public:
   ForwardPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager, const ForwardLighting* forwardLighting);
   ~ForwardPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& colorTexture, Texture* colorResolveTexture, Texture& roughnessMetalnessTexture, Texture& normalTexture, Texture& ssaoTexture, const Texture* skyboxTexture);

protected:
   friend class SceneRenderPass<ForwardPass>;

   bool supportsMaterialType(uint32_t typeMask) const;

   void renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;

   PipelineDescription<ForwardPass> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const;
   Pipeline createPipeline(const PipelineDescription<ForwardPass>& description, const AttachmentFormats& attachmentFormats);

private:
   ForwardShader* forwardShader = nullptr;
   SkyboxShader* skyboxShader = nullptr;

   vk::PipelineLayout forwardPipelineLayout;
   vk::PipelineLayout skyboxPipelineLayout;

   ForwardDescriptorSet forwardDescriptorSet;
   vk::Sampler normalSampler;

   const ForwardLighting* lighting = nullptr;
   SkyboxDescriptorSet skyboxDescriptorSet;
   vk::Sampler skyboxSampler;
};
