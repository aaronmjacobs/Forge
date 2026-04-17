#pragma once

#include "Core/Enum.h"
#include "Core/Hash.h"

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

   alignas(4) float paperWhiteNits = 0.0f;
   alignas(4) float peakBrightnessNits = 0.0f;

   alignas(4) float toe = 0.0f;
   alignas(4) float shoulder = 0.0f;
   alignas(4) float hotspot = 0.0f;
   alignas(4) float huePreservation = 0.0f;
};

template<>
struct PipelineDescription<TonemapPass>
{
   TonemapShaderConstants shaderConstants;

   std::size_t hash() const
   {
      return Hash::of(
         shaderConstants.tonemappingAlgorithm,
         shaderConstants.colorGamut,
         shaderConstants.transferFunction,
         shaderConstants.outputHDR,
         shaderConstants.withBloom,
         shaderConstants.withUI,
         shaderConstants.showTestPattern);
   }

   bool operator==(const PipelineDescription<TonemapPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<TonemapPass>);

class TonemapPass : public SceneRenderPass<TonemapPass>
{
public:
   TonemapPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~TonemapPass();

   void render(vk::CommandBuffer commandBuffer, Texture& outputTexture, Texture& hdrColorTexture, Texture* bloomTexture, Texture* uiTexture, const TonemapSettings& settings, vk::ColorSpaceKHR outputColorSpace);

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
