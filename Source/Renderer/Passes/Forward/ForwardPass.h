#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

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
   bool skybox = false;

   std::size_t hash() const
   {
      return (withTextures * 0b001) | (withBlending * 0b010) | (skybox * 0b100);
   }

   bool operator==(const PipelineDescription<ForwardPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<ForwardPass>);

class ForwardPass : public SceneRenderPass<ForwardPass>
{
public:
   ForwardPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager, const ForwardLighting* forwardLighting);
   ~ForwardPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle, const Texture* skyboxTexture);

protected:
   friend class SceneRenderPass<ForwardPass>;

   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   void renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::PipelineLayout selectPipelineLayout(BlendMode blendMode) const;

   PipelineDescription<ForwardPass> getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const;
   vk::Pipeline createPipeline(const PipelineDescription<ForwardPass>& description);

private:
   std::unique_ptr<ForwardShader> forwardShader;
   std::unique_ptr<SkyboxShader> skyboxShader;

   vk::PipelineLayout forwardPipelineLayout;
   vk::PipelineLayout skyboxPipelineLayout;

   const ForwardLighting* lighting = nullptr;
   DescriptorSet skyboxDescriptorSet;
   vk::Sampler skyboxSampler;
};
