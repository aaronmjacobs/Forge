#include "Renderer/Passes/Composite/CompositePass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include <utility>

namespace
{
   const char* getModeName(CompositeShader::Mode mode)
   {
      switch (mode)
      {
      case CompositeShader::Mode::Passthrough:
         return "Passthrough";
      case CompositeShader::Mode::LinearToSrgb:
         return "LinearToSrgb";
      case CompositeShader::Mode::SrgbToLinear:
         return "SrgbToLinear";
      default:
         ASSERT(false);
         return nullptr;
      }
   }
}

CompositePass::CompositePass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, CompositeShader::getLayoutCreateInfo())
{
   compositeShader = std::make_unique<CompositeShader>(context, resourceManager);

   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = compositeShader->getSetLayouts();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts);
   pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
   NAME_CHILD(pipelineLayout, "Pipeline Layout");

   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eNearest)
      .setMinFilter(vk::Filter::eNearest)
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
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
}

CompositePass::~CompositePass()
{
   context.delayedDestroy(std::move(sampler));
   context.delayedDestroy(std::move(pipelineLayout));

   compositeShader.reset();
}

void CompositePass::render(vk::CommandBuffer commandBuffer, FramebufferHandle framebufferHandle, Texture& sourceTexture, CompositeShader::Mode mode)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   vk::ImageLayout initialLayout = sourceTexture.getLayout();
   if (initialLayout != vk::ImageLayout::eShaderReadOnlyOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      sourceTexture.transitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }

   beginRenderPass(commandBuffer, *framebuffer, std::span<vk::ClearValue>{});

   vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
      .setImageLayout(sourceTexture.getLayout())
      .setImageView(sourceTexture.getDefaultView())
      .setSampler(sampler);
   vk::WriteDescriptorSet descriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(descriptorSet.getCurrentSet())
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&imageInfo);
   device.updateDescriptorSets(descriptorWrite, {});

   PipelineDescription<CompositePass> pipelineDescription;
   pipelineDescription.mode = mode;

   compositeShader->bindDescriptorSets(commandBuffer, pipelineLayout, descriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);

   if (initialLayout == vk::ImageLayout::eColorAttachmentOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      sourceTexture.transitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }
}

std::vector<vk::SubpassDependency> CompositePass::getSubpassDependencies() const
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

Pipeline CompositePass::createPipeline(const PipelineDescription<CompositePass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(true)
      .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData;
   pipelineData.renderPass = getRenderPass();
   pipelineData.layout = pipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.shaderStages = compositeShader->getStages(description.mode);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, getModeName(description.mode));

   return pipeline;
}
