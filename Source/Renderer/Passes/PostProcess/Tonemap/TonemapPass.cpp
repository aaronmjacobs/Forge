#include "Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include <utility>

TonemapPass::TonemapPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, TonemapShader::getLayoutCreateInfo())
{
   clearDepth = false;
   clearColor = true;

   tonemapShader = std::make_unique<TonemapShader>(context, resourceManager);

   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = tonemapShader->getSetLayouts();
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

TonemapPass::~TonemapPass()
{
   context.delayedDestroy(std::move(sampler));
   context.delayedDestroy(std::move(pipelineLayout));

   tonemapShader.reset();
}

void TonemapPass::render(vk::CommandBuffer commandBuffer, FramebufferHandle framebufferHandle, Texture& hdrColorTexture)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      hdrColorTexture.transitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }

   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
   std::array<vk::ClearValue, 1> clearValues = { vk::ClearColorValue(clearColorValues) };

   beginRenderPass(commandBuffer, *framebuffer, clearValues);

   vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
      .setImageLayout(hdrColorTexture.getLayout())
      .setImageView(hdrColorTexture.getDefaultView())
      .setSampler(sampler);
   vk::WriteDescriptorSet descriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(descriptorSet.getCurrentSet())
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&imageInfo);
   device.updateDescriptorSets(descriptorWrite, {});

   PipelineDescription<TonemapPass> pipelineDescription;
   const AttachmentInfo& attachmentInfo = framebuffer->getAttachmentInfo();
   pipelineDescription.hdr = attachmentInfo.colorInfo.size() > 0 && attachmentInfo.colorInfo[0].format == vk::Format::eA2R10G10B10UnormPack32;

   tonemapShader->bindDescriptorSets(commandBuffer, pipelineLayout, descriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);

   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      hdrColorTexture.transitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }
}

std::vector<vk::SubpassDependency> TonemapPass::getSubpassDependencies() const
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

vk::Pipeline TonemapPass::createPipeline(const PipelineDescription<TonemapPass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.renderPass = getRenderPass();
   pipelineInfo.layout = pipelineLayout;
   pipelineInfo.sampleCount = getSampleCount();
   pipelineInfo.passType = PipelinePassType::Screen;
   pipelineInfo.enableDepthTest = false;
   pipelineInfo.writeDepth = false;
   pipelineInfo.positionOnly = false;

   PipelineData pipelineData(pipelineInfo, tonemapShader->getStages(description.hdr), { attachmentState });
   vk::Pipeline pipeline = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipeline, std::string("Pipeline ") + (description.hdr ? "(HDR)" : "(SDR)"));

   return pipeline;
}
