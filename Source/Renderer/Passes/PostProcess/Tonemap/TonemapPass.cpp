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

void TonemapPass::render(vk::CommandBuffer commandBuffer, Texture& outputTexture, Texture& hdrColorTexture)
{
   SCOPED_LABEL(getName());

   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   hdrColorTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);

   vk::RenderingAttachmentInfo colorAttachmentInfo = vk::RenderingAttachmentInfo()
      .setImageView(outputTexture.getDefaultView())
      .setImageLayout(outputTexture.getLayout())
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   beginRenderPass(commandBuffer, outputTexture.getExtent(), nullptr, &colorAttachmentInfo);

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
   pipelineDescription.hdr = outputTexture.getImageProperties().format == vk::Format::eA2R10G10B10UnormPack32;

   tonemapShader->bindDescriptorSets(commandBuffer, pipelineLayout, descriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}

Pipeline TonemapPass::createPipeline(const PipelineDescription<TonemapPass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData;
   pipelineData.layout = pipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.depthStencilFormat = getDepthStencilFormat();
   pipelineData.colorFormats = getColorFormats();
   pipelineData.shaderStages = tonemapShader->getStages(description.hdr);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.hdr ? "HDR" : "SDR");

   return pipeline;
}
