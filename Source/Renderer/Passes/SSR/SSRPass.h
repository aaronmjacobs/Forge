#pragma once

#include "Core/Enum.h"

#include "Renderer/Passes/SceneRenderPass.h"
#include "Renderer/Passes/SSR/SSRGenerateShader.h"

#include <memory>

class DynamicDescriptorPool;
class ResourceManager;
class Texture;
class SSRPass;
struct SceneRenderInfo;

enum class SSRPassStage
{
   Generate = 0b00,
   Blur = 0b01,
   Apply = 0b10
};

template<>
struct PipelineDescription<SSRPass>
{
   SSRPassStage stage = SSRPassStage::Generate;

   std::size_t hash() const
   {
      return Enum::cast(stage);
   }

   bool operator==(const PipelineDescription<SSRPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<SSRPass>);

class SSRPass : public SceneRenderPass<SSRPass>
{
public:
   SSRPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~SSRPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture);

   void recreateTextures(vk::SampleCountFlagBits sampleCount);

   Texture* getReflectedUvTexture() const
   {
      return reflectedUvTexture.get();
   }

protected:
   friend class SceneRenderPass<SSRPass>;

   Pipeline createPipeline(const PipelineDescription<SSRPass>& description, const AttachmentFormats& attachmentFormats);

private:
   void generate(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture);

   void createTextures(vk::SampleCountFlagBits sampleCount);
   void destroyTextures();

   vk::ImageView getDepthView(Texture& depthTexture);

   SSRGenerateShader* generateShader = nullptr;

   vk::PipelineLayout generatePipelineLayout;

   SSRGenerateDescriptorSet generateDescriptorSet;

   vk::Sampler sampler;

   std::unique_ptr<Texture> reflectedUvTexture;
};
