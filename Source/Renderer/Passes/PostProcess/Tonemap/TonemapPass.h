#pragma once

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/Passes/SceneRenderPass.h"
#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"
#include "Renderer/RenderSettings.h"

#include <memory>

class DynamicDescriptorPool;
class ResourceManager;
class Texture;
class TonemapPass;

struct TonemapUniformData
{
   alignas(4) float bloomStrength = 0.0f;
   alignas(4) float peakBrightness = 0.0f;

   alignas(4) float shoulder = 0.0f;
   alignas(4) float hotspot = 0.0f;
   alignas(4) float hotspotSlope = 0.0f;
   alignas(4) float huePreservation = 0.0f;
};

template<>
struct PipelineDescription<TonemapPass>
{
   TonemappingAlgorithm tonemappingAlgorithm = TonemappingAlgorithm::None;
   bool hdr = false;
   bool withBloom = false;
   bool withUI = false;
   bool showTestPattern = false;

   std::size_t hash() const
   {
      return (Enum::cast(tonemappingAlgorithm) << 4) | (hdr * 0b1000) | (withBloom * 0b0100) | (withUI * 0b0010) | (showTestPattern * 0b0001);
   }

   bool operator==(const PipelineDescription<TonemapPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<TonemapPass>);

class TonemapPass : public SceneRenderPass<TonemapPass>
{
public:
   TonemapPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~TonemapPass();

   void render(vk::CommandBuffer commandBuffer, Texture& outputTexture, Texture& hdrColorTexture, Texture* bloomTexture, Texture* uiTexture, const TonemapSettings& settings);

protected:
   friend class SceneRenderPass<TonemapPass>;

   Pipeline createPipeline(const PipelineDescription<TonemapPass>& description, const AttachmentFormats& attachmentFormats);

private:
   TonemapShader* tonemapShader = nullptr;

   vk::PipelineLayout pipelineLayout;

   TonemapDescriptorSet descriptorSet;
   vk::Sampler sampler;

   StrongTextureHandle lutTextureHandle;

   UniformBuffer<TonemapUniformData> uniformBuffer;
};
