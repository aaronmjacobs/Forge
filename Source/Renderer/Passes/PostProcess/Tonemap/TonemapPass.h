#pragma once

#include "Graphics/DescriptorSet.h"

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DynamicDescriptorPool;
class ResourceManager;
class Texture;
class TonemapPass;
class TonemapShader;

template<>
struct PipelineDescription<TonemapPass>
{
   bool hdr = false;
   bool withBloom = false;
   bool withUI = false;

   std::size_t hash() const
   {
      return (hdr * 0b001) | (withBloom * 0b010) | (withUI * 0b100);
   }

   bool operator==(const PipelineDescription<TonemapPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<TonemapPass>);

class TonemapPass : public SceneRenderPass<TonemapPass>
{
public:
   TonemapPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~TonemapPass();

   void render(vk::CommandBuffer commandBuffer, Texture& outputTexture, Texture& hdrColorTexture, Texture* bloomTexture, Texture* uiTexture);

protected:
   friend class SceneRenderPass<TonemapPass>;

   Pipeline createPipeline(const PipelineDescription<TonemapPass>& description);

private:
   std::unique_ptr<TonemapShader> tonemapShader;

   vk::PipelineLayout pipelineLayout;

   DescriptorSet descriptorSet;
   vk::Sampler sampler;
};
