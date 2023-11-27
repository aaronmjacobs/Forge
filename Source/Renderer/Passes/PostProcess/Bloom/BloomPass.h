#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/Passes/SceneRenderPass.h"
#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"
#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"
#include "Renderer/RenderSettings.h"

#include <array>
#include <memory>
#include <vector>

class BloomPass;
class DynamicDescriptorPool;
class ResourceManager;
class Texture;

struct BloomUpsampleUniformData
{
   alignas(4) float filterRadius = 0.0f;
   alignas(4) float colorMix = 0.0f;
};

enum class BloomPassType
{
   Downsample,
   HorizontalUpsample,
   VerticalUpsample
};

template<>
struct PipelineDescription<BloomPass>
{
   BloomPassType type = BloomPassType::Downsample;
   RenderQuality quality = RenderQuality::High;

   std::size_t hash() const
   {
      return (static_cast<std::size_t>(type) << 2) | static_cast<std::size_t>(quality);
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

   void render(vk::CommandBuffer commandBuffer, Texture& hdrColorTexture, Texture& defaultBlackTexture, RenderQuality quality);

   void recreateTextures(vk::Format format, vk::SampleCountFlagBits sampleCount);

   Texture* getOutputTexture() const
   {
      return textures[0].get();
   }

protected:
   friend class SceneRenderPass<BloomPass>;

   Pipeline createPipeline(const PipelineDescription<BloomPass>& description, const AttachmentFormats& attachmentFormats);

private:
   void renderDownsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& hdrColorTexture, RenderQuality quality);
   void renderUpsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& defaultBlackTexture, RenderQuality quality, bool horizontal);

   void createTextures(vk::Format format, vk::SampleCountFlagBits sampleCount);
   void destroyTextures();

   BloomDownsampleShader* downsampleShader = nullptr;
   BloomUpsampleShader* upsampleShader = nullptr;

   vk::PipelineLayout downsamplePipelineLayout;
   vk::PipelineLayout upsamplePipelineLayout;

   std::vector<BloomDownsampleDescriptorSet> downsampleDescriptorSets;
   std::vector<BloomUpsampleDescriptorSet> horizontalUpsampleDescriptorSets;
   std::vector<BloomUpsampleDescriptorSet> verticalUpsampleDescriptorSets;
   vk::Sampler sampler;

   std::vector<UniformBuffer<BloomUpsampleUniformData>> upsampleUniformBuffers;

   std::array<std::unique_ptr<Texture>, kNumSteps> textures;
   std::array<std::unique_ptr<Texture>, kNumSteps> horizontalBlurTextures;
};
