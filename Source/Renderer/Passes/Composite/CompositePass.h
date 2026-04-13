#pragma once

#include "Core/Enum.h"
#include "Core/Hash.h"

#include "Renderer/Passes/Composite/CompositeShader.h"
#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class CompositePass;
class DynamicDescriptorPool;
class ResourceManager;
class Texture;

template<>
struct PipelineDescription<CompositePass>
{
   CompositeShaderConstants shaderConstants;

   std::size_t hash() const
   {
      return Hash::of(shaderConstants.mode);
   }

   bool operator==(const PipelineDescription<CompositePass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<CompositePass>);

class CompositePass : public SceneRenderPass<CompositePass>
{
public:
   CompositePass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~CompositePass();

   void render(vk::CommandBuffer commandBuffer, Texture& destinationTexture, Texture& sourceTexture, CompositeMode mode);

protected:
   friend class SceneRenderPass<CompositePass>;

   Pipeline createPipeline(const PipelineDescription<CompositePass>& description, const AttachmentFormats& attachmentFormats);

private:
   CompositeShader* compositeShader = nullptr;

   vk::PipelineLayout pipelineLayout;

   CompositeDescriptorSet descriptorSet;
   vk::Sampler sampler;
};
