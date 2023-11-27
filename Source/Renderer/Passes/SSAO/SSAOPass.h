#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/FrameData.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/RenderSettings.h"

#include "Renderer/Passes/SceneRenderPass.h"
#include "Renderer/Passes/SSAO/SSAOBlurShader.h"
#include "Renderer/Passes/SSAO/SSAOShader.h"

#include <glm/glm.hpp>

#include <array>
#include <memory>

class DynamicDescriptorPool;
class ResourceManager;
class Texture;
class SSAOPass;
class SSAOBlurShader;
class SSAOShader;
struct SceneRenderInfo;

struct SSAOUniformData
{
   alignas(16) std::array<glm::vec4, 32> samples;
   alignas(16) std::array<glm::vec4, 8> noise;
   alignas(4) uint32_t numSamples = 0;
};

template<>
struct PipelineDescription<SSAOPass>
{
   bool blur = false;
   bool horizontal = false;

   std::size_t hash() const
   {
      return (blur * 0b01) | (horizontal * 0b10);
   }

   bool operator==(const PipelineDescription<SSAOPass>& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(PipelineDescription<SSAOPass>);

class SSAOPass : public SceneRenderPass<SSAOPass>
{
public:
   SSAOPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager);
   ~SSAOPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture, Texture& ssaoTexture, Texture& ssaoBlurTexture, RenderQuality quality);

protected:
   friend class SceneRenderPass<SSAOPass>;

   Pipeline createPipeline(const PipelineDescription<SSAOPass>& description, const AttachmentFormats& attachmentFormats);

private:
   void renderSSAO(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture, Texture& ssaoTexture, RenderQuality quality);
   void renderBlur(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& inputTexture, Texture& outputTexture, bool horizontal);

   vk::ImageView getDepthView(Texture& depthTexture);

   SSAOShader* ssaoShader = nullptr;
   SSAOBlurShader* blurShader = nullptr;

   vk::PipelineLayout ssaoPipelineLayout;
   vk::PipelineLayout blurPipelineLayout;

   SSAODescriptorSet ssaoDescriptorSet;
   SSAOBlurDescriptorSet horizontalBlurDescriptorSet;
   SSAOBlurDescriptorSet verticalBlurDescriptorSet;

   vk::Sampler sampler;

   UniformBuffer<SSAOUniformData> uniformBuffer;
   FrameData<RenderQuality> ssaoQuality;
};
