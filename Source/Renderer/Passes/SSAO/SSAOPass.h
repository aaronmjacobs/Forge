#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/Passes/SceneRenderPass.h"

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
   alignas(16) std::array<glm::vec4, 64> samples;
   alignas(16) std::array<glm::vec4, 16> noise;
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

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle ssaoFramebufferHandle, FramebufferHandle blurFramebufferHandle, Texture& depthBuffer, Texture& normalBuffer, Texture& ssaoBuffer, Texture& ssaoBlurBuffer);

protected:
   friend class SceneRenderPass<SSAOPass>;

   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   Pipeline createPipeline(const PipelineDescription<SSAOPass>& description);

private:
   void renderSSAO(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, const Framebuffer& framebuffer, Texture& depthBuffer, Texture& normalBuffer);
   void renderBlur(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, const Framebuffer& framebuffer, Texture& sourceBuffer, Texture& depthBuffer, bool horizontal);

   std::unique_ptr<SSAOShader> ssaoShader;
   std::unique_ptr<SSAOBlurShader> blurShader;

   vk::PipelineLayout ssaoPipelineLayout;
   vk::PipelineLayout blurPipelineLayout;

   DescriptorSet ssaoDescriptorSet;
   DescriptorSet horizontalBlurDescriptorSet;
   DescriptorSet verticalBlurDescriptorSet;

   vk::Sampler sampler;

   UniformBuffer<SSAOUniformData> uniformBuffer;
};
