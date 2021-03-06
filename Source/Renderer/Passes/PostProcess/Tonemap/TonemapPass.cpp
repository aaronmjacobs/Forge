#include "Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

TonemapPass::TonemapPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, TonemapShader::getLayoutCreateInfo())
{
   clearDepth = false;
   clearColor = true;
   pipelines.resize(1);

   tonemapShader = std::make_unique<TonemapShader>(context, resourceManager);

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
}

TonemapPass::~TonemapPass()
{
   device.destroySampler(sampler);

   tonemapShader.reset();
}

void TonemapPass::render(vk::CommandBuffer commandBuffer, Texture& hdrColorTexture)
{
   SCOPED_LABEL("Tonemap pass");

   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      hdrColorTexture.transitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }

   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
   std::array<vk::ClearValue, 1> clearValues = { vk::ClearColorValue(clearColorValues) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(getRenderPass())
      .setFramebuffer(getCurrentFramebuffer())
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), getFramebufferExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

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

   tonemapShader->bindDescriptorSets(commandBuffer, pipelineLayout, descriptorSet);
   renderScreenMesh(commandBuffer, pipelines[0]);

   commandBuffer.endRenderPass();

   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      hdrColorTexture.transitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }
}

#if FORGE_DEBUG
void TonemapPass::setName(std::string_view newName)
{
   SceneRenderPass::setName(newName);

   NAME_OBJECT(pipelines[0], name + " Pipeline");
}
#endif // FORGE_DEBUG

void TonemapPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = tonemapShader->getSetLayouts();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts);
   pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineData pipelineData(context, pipelineLayout, getRenderPass(), PipelinePassType::Screen, tonemapShader->getStages(), { attachmentState }, sampleCount);
   pipelines[0] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
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

void TonemapPass::postUpdateAttachments()
{
   NAME_OBJECT(*this, "Tonemap Pass");
}
