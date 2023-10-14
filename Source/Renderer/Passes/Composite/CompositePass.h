#pragma once

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"

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
   CompositeShader::Mode mode = CompositeShader::Mode::Passthrough;

   std::size_t hash() const
   {
      return Enum::cast(mode);
   }

   bool operator==(const PipelineDescription<CompositePass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<CompositePass>);

class CompositePass : public SceneRenderPass<CompositePass>
{
public:
   CompositePass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~CompositePass();

   void render(vk::CommandBuffer commandBuffer, Texture& destinationTexture, Texture& sourceTexture, CompositeShader::Mode mode);

protected:
   friend class SceneRenderPass<CompositePass>;

   Pipeline createPipeline(const PipelineDescription<CompositePass>& description, const AttachmentFormats& attachmentFormats);

private:
   CompositeShader* compositeShader = nullptr;

   vk::PipelineLayout pipelineLayout;

   DescriptorSet descriptorSet;
   vk::Sampler sampler;
};
