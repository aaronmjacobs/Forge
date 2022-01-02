#include "Renderer/Passes/Composite/CompositePass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include <utility>

CompositePass::CompositePass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, CompositeShader::getLayoutCreateInfo())
{
   pipelines.resize(CompositeShader::kNumModes);

   compositeShader = std::make_unique<CompositeShader>(context, resourceManager);

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

   compositeShader->bindDescriptorSets(commandBuffer, pipelineLayouts[0], descriptorSet);
   renderScreenMesh(commandBuffer, pipelines[Enum::cast(mode)]);

   endRenderPass(commandBuffer);

   if (initialLayout == vk::ImageLayout::eColorAttachmentOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      sourceTexture.transitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }
}

void CompositePass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   pipelineLayouts.resize(1);

   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = compositeShader->getSetLayouts();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts);
   pipelineLayouts[0] = device.createPipelineLayout(pipelineLayoutCreateInfo);

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
   pipelineInfo.renderPass = getRenderPass();
   pipelineInfo.layout = pipelineLayouts[0];
   pipelineInfo.sampleCount = sampleCount;
   pipelineInfo.passType = PipelinePassType::Screen;
   pipelineInfo.enableDepthTest = false;
   pipelineInfo.writeDepth = false;
   pipelineInfo.positionOnly = false;

   PipelineData pipelineData(context, pipelineInfo, compositeShader->getStages(CompositeShader::Mode::Passthrough), { attachmentState });
   pipelines[Enum::cast(CompositeShader::Mode::Passthrough)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[Enum::cast(CompositeShader::Mode::Passthrough)], "Pipeline (Passthrough)");

   pipelineData.setShaderStages(compositeShader->getStages(CompositeShader::Mode::LinearToSrgb));
   pipelines[Enum::cast(CompositeShader::Mode::LinearToSrgb)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[Enum::cast(CompositeShader::Mode::LinearToSrgb)], "Pipeline (LinearToSrgb)");

   pipelineData.setShaderStages(compositeShader->getStages(CompositeShader::Mode::SrgbToLinear));
   pipelines[Enum::cast(CompositeShader::Mode::SrgbToLinear)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[Enum::cast(CompositeShader::Mode::SrgbToLinear)], "Pipeline (SrgbToLinear)");
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
