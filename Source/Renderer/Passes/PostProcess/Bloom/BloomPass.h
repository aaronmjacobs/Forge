#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/Passes/SceneRenderPass.h"

#include <array>
#include <memory>
#include <vector>

class BloomDownsampleShader;
class BloomPass;
class BloomUpsampleShader;
class DynamicDescriptorPool;
class ResourceManager;
class Texture;

struct BloomUpsampleUniformData
{
   alignas(4) float filterRadius = 0.0f;
   alignas(4) float colorMix = 0.0f;
};

template<>
struct PipelineDescription<BloomPass>
{
   bool upsample = false;

   std::size_t hash() const
   {
      return (upsample * 0b01);
   }

   bool operator==(const PipelineDescription<BloomPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<BloomPass>);

class BloomPass : public SceneRenderPass<BloomPass>
{
public:
   static constexpr uint32_t kNumSteps = 5;

   BloomPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~BloomPass();

   void render(vk::CommandBuffer commandBuffer, Texture& hdrColorTexture, Texture& defaultBlackTexture);

   Texture* getOutputTexture() const
   {
      return upsampleTextures[0].get();
   }

protected:
   friend class SceneRenderPass<BloomPass>;

   void postUpdateAttachmentFormats() override;

   Pipeline createPipeline(const PipelineDescription<BloomPass>& description);

private:
   void renderDownsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& hdrColorTexture);
   void renderUpsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& defaultBlackTexture);

   void createTextures();
   void destroyTextures();

   std::unique_ptr<BloomDownsampleShader> downsampleShader;
   std::unique_ptr<BloomUpsampleShader> upsampleShader;

   vk::PipelineLayout downsamplePipelineLayout;
   vk::PipelineLayout upsamplePipelineLayout;

   std::vector<DescriptorSet> downsampleDescriptorSets;
   std::vector<DescriptorSet> upsampleDescriptorSets;
   vk::Sampler sampler;

   std::vector<UniformBuffer<BloomUpsampleUniformData>> upsampleUniformBuffers;

   std::array<std::unique_ptr<Texture>, kNumSteps> downsampleTextures;
   std::array<std::unique_ptr<Texture>, kNumSteps> upsampleTextures;
};
