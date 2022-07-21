#include "Renderer/Passes/SSAO/SSAOPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/SSAO/SSAOBlurShader.h"
#include "Renderer/Passes/SSAO/SSAOShader.h"
#include "Renderer/SceneRenderInfo.h"

#include <glm/glm.hpp>

#include <random>
#include <utility>

class ScopedTextureLayoutTransition
{
public:
   ScopedTextureLayoutTransition(vk::CommandBuffer cmdBuffer, Texture& tex, vk::ImageLayout desiredLayout, const TextureMemoryBarrierFlags& startSrcFlags, const TextureMemoryBarrierFlags& startDstFlags, const TextureMemoryBarrierFlags& endSrcFlags, const TextureMemoryBarrierFlags& endDstFlags)
      : commandBuffer(cmdBuffer)
      , texture(tex)
      , initialLayout(tex.getLayout())
      , startSrcBarrierFlags(startSrcFlags)
      , startDstBarrierFlags(startDstFlags)
      , endSrcBarrierFlags(endSrcFlags)
      , endDstBarrierFlags(endDstFlags)
   {
      if (initialLayout != desiredLayout)
      {
         texture.transitionLayout(commandBuffer, desiredLayout, startSrcBarrierFlags, startDstBarrierFlags);
      }
   }

   ~ScopedTextureLayoutTransition()
   {
      if (texture.getLayout() != initialLayout)
      {
         texture.transitionLayout(commandBuffer, initialLayout, endSrcBarrierFlags, endDstBarrierFlags);
      }
   }

private:
   vk::CommandBuffer commandBuffer;
   Texture& texture;

   vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;

   TextureMemoryBarrierFlags startSrcBarrierFlags;
   TextureMemoryBarrierFlags startDstBarrierFlags;
   TextureMemoryBarrierFlags endSrcBarrierFlags;
   TextureMemoryBarrierFlags endDstBarrierFlags;
};

SSAOPass::SSAOPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , ssaoDescriptorSet(graphicsContext, dynamicDescriptorPool, SSAOShader::getLayoutCreateInfo())
   , horizontalBlurDescriptorSet(graphicsContext, dynamicDescriptorPool, SSAOBlurShader::getLayoutCreateInfo())
   , verticalBlurDescriptorSet(graphicsContext, dynamicDescriptorPool, SSAOBlurShader::getLayoutCreateInfo())
   , uniformBuffer(graphicsContext)
{
   clearColor = true;
   clearDepth = false;

   NAME_CHILD(ssaoDescriptorSet, "SSAO");
   NAME_CHILD(blurPipelineLayout, "Blur");
   NAME_CHILD(uniformBuffer, "");

   ssaoShader = std::make_unique<SSAOShader>(context, resourceManager);
   blurShader = std::make_unique<SSAOBlurShader>(context, resourceManager);

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = ssaoShader->getSetLayouts();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts);
      ssaoPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(ssaoPipelineLayout, "SSAO Pipeline Layout");
   }

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = blurShader->getSetLayouts();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts);
      blurPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(blurPipelineLayout, "Blur Pipeline Layout");
   }

   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eNearest)
      .setMinFilter(vk::Filter::eNearest)
      .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
      .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
      .setAnisotropyEnable(false)
      .setMaxAnisotropy(1.0f)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eNearest)
      .setMipLodBias(0.0f)
      .setMinLod(0.0f)
      .setMaxLod(16.0f);
   sampler = context.getDevice().createSampler(samplerCreateInfo);
   NAME_CHILD(sampler, "Sampler");

   {
      SSAOUniformData uniformData;

      std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
      std::default_random_engine generator;

      for (uint32_t i = 0; i < uniformData.samples.size(); ++i)
      {
         glm::vec4& sample = uniformData.samples[i];
         sample = glm::vec4(glm::normalize(glm::vec3(distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator))), 0.0f);
         sample *= distribution(generator);

         float scale = static_cast<float>(i) / uniformData.samples.size();
         scale = glm::mix(0.1f, 1.0f, scale * scale);
         sample *= scale;
      }

      for (glm::vec4& noiseValue : uniformData.noise)
      {
         noiseValue = glm::vec4(distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f);
      }

      uniformBuffer.updateAll(uniformData);

      for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
      {
         vk::DescriptorBufferInfo bufferInfo = uniformBuffer.getDescriptorBufferInfo(frameIndex);

         vk::WriteDescriptorSet bufferDescriptorWrite = vk::WriteDescriptorSet()
            .setDstSet(ssaoDescriptorSet.getSet(frameIndex))
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&bufferInfo);

         device.updateDescriptorSets({ bufferDescriptorWrite }, {});
      }
   }
}

SSAOPass::~SSAOPass()
{
   context.delayedDestroy(std::move(sampler));

   context.delayedDestroy(std::move(ssaoPipelineLayout));
   context.delayedDestroy(std::move(blurPipelineLayout));

   ssaoShader.reset();
   blurShader.reset();
}

void SSAOPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle ssaoFramebufferHandle, FramebufferHandle blurFramebufferHandle, Texture& depthBuffer, Texture& normalBuffer, Texture& ssaoBuffer, Texture& ssaoBlurBuffer)
{
   SCOPED_LABEL(getName());

   const Framebuffer* ssaoFramebuffer = getFramebuffer(ssaoFramebufferHandle);
   const Framebuffer* blurFramebuffer = getFramebuffer(blurFramebufferHandle);
   if (!ssaoFramebuffer || !blurFramebuffer)
   {
      ASSERT(false);
      return;
   }

   renderSSAO(commandBuffer, sceneRenderInfo, *ssaoFramebuffer, depthBuffer, normalBuffer);

   renderBlur(commandBuffer, sceneRenderInfo, *blurFramebuffer, ssaoBuffer, depthBuffer, true);
   renderBlur(commandBuffer, sceneRenderInfo, *ssaoFramebuffer, ssaoBlurBuffer, depthBuffer, false);
}

std::vector<vk::SubpassDependency> SSAOPass::getSubpassDependencies() const
{
   std::vector<vk::SubpassDependency> subpassDependencies;

   subpassDependencies.push_back(vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite));

   return subpassDependencies;
}

Pipeline SSAOPass::createPipeline(const PipelineDescription<SSAOPass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData;
   pipelineData.renderPass = getRenderPass();
   pipelineData.layout = description.blur ? blurPipelineLayout : ssaoPipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.shaderStages = description.blur ? blurShader->getStages(description.horizontal) : ssaoShader->getStages();
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.blur ? (description.horizontal ? "Blur (Horizontal)" : "Blur (Vertical)") : "SSAO");

   return pipeline;
}

void SSAOPass::renderSSAO(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, const Framebuffer& framebuffer, Texture& depthBuffer, Texture& normalBuffer)
{
   SCOPED_LABEL("SSAO");

   bool depthViewCreated = false;
   vk::ImageView depthView = depthBuffer.getOrCreateView(vk::ImageViewType::e2D, 0, 1, vk::ImageAspectFlagBits::eDepth, &depthViewCreated);
   if (depthViewCreated)
   {
      NAME_CHILD(depthView, "Depth View");
   }

   static const TextureMemoryBarrierFlags kDepthWriteFlags(vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eLateFragmentTests);
   static const TextureMemoryBarrierFlags kColorWriteFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
   static const TextureMemoryBarrierFlags kShaderReadFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);

   ScopedTextureLayoutTransition depthBufferTransition(commandBuffer, depthBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, kDepthWriteFlags, kShaderReadFlags, kShaderReadFlags, kDepthWriteFlags);
   ScopedTextureLayoutTransition normalBufferTransition(commandBuffer, normalBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, kColorWriteFlags, kShaderReadFlags, kShaderReadFlags, kColorWriteFlags);

   std::array<float, 4> clearColorValues = { 1.0f, 1.0f, 1.0f, 1.0f };
   std::array<vk::ClearValue, 1> clearValues = { vk::ClearColorValue(clearColorValues) };
   beginRenderPass(commandBuffer, framebuffer, clearValues);

   vk::DescriptorImageInfo depthBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(depthBuffer.getLayout())
      .setImageView(depthView)
      .setSampler(sampler);
   vk::WriteDescriptorSet depthBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(ssaoDescriptorSet.getCurrentSet())
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&depthBufferImageInfo);
   vk::DescriptorImageInfo normalBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(normalBuffer.getLayout())
      .setImageView(normalBuffer.getDefaultView())
      .setSampler(sampler);
   vk::WriteDescriptorSet normalBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(ssaoDescriptorSet.getCurrentSet())
      .setDstBinding(1)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&normalBufferImageInfo);
   device.updateDescriptorSets({ depthBufferDescriptorWrite, normalBufferDescriptorWrite }, {});

   PipelineDescription<SSAOPass> pipelineDescription;
   pipelineDescription.blur = false;

   ssaoShader->bindDescriptorSets(commandBuffer, ssaoPipelineLayout, sceneRenderInfo.view, ssaoDescriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}

void SSAOPass::renderBlur(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, const Framebuffer& framebuffer, Texture& sourceBuffer, Texture& depthBuffer, bool horizontal)
{
   SCOPED_LABEL(std::string(horizontal ? "Horizontal" : "Vertical") + " Blur");

   bool depthViewCreated = false;
   vk::ImageView depthView = depthBuffer.getOrCreateView(vk::ImageViewType::e2D, 0, 1, vk::ImageAspectFlagBits::eDepth, &depthViewCreated);
   if (depthViewCreated)
   {
      NAME_CHILD(depthView, "Depth View");
   }

   static const TextureMemoryBarrierFlags kDepthWriteFlags(vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eLateFragmentTests);
   static const TextureMemoryBarrierFlags kColorWriteFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
   static const TextureMemoryBarrierFlags kShaderReadFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);

   ScopedTextureLayoutTransition depthBufferTransition(commandBuffer, depthBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, kDepthWriteFlags, kShaderReadFlags, kShaderReadFlags, kDepthWriteFlags);
   ScopedTextureLayoutTransition sourceBufferTransition(commandBuffer, sourceBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, kColorWriteFlags, kShaderReadFlags, kShaderReadFlags, kColorWriteFlags);

   std::array<float, 4> clearColorValues = { 1.0f, 1.0f, 1.0f, 1.0f };
   std::array<vk::ClearValue, 1> clearValues = { vk::ClearColorValue(clearColorValues) };
   beginRenderPass(commandBuffer, framebuffer, clearValues);

   DescriptorSet& blurDescriptorSet = horizontal ? horizontalBlurDescriptorSet : verticalBlurDescriptorSet;

   vk::DescriptorImageInfo sourceBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(sourceBuffer.getLayout())
      .setImageView(sourceBuffer.getDefaultView())
      .setSampler(sampler);
   vk::WriteDescriptorSet sourceBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(blurDescriptorSet.getCurrentSet())
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&sourceBufferImageInfo);
   vk::DescriptorImageInfo depthBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(depthBuffer.getLayout())
      .setImageView(depthView)
      .setSampler(sampler);
   vk::WriteDescriptorSet depthBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(blurDescriptorSet.getCurrentSet())
      .setDstBinding(1)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&depthBufferImageInfo);
   device.updateDescriptorSets({ sourceBufferDescriptorWrite, depthBufferDescriptorWrite }, {});

   PipelineDescription<SSAOPass> pipelineDescription;
   pipelineDescription.blur = true;
   pipelineDescription.horizontal = horizontal;

   blurShader->bindDescriptorSets(commandBuffer, blurPipelineLayout, sceneRenderInfo.view, blurDescriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}
