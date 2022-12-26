#include "Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"

#include "Core/Containers/StaticVector.h"

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
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
      .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
      .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
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

void TonemapPass::render(vk::CommandBuffer commandBuffer, Texture& outputTexture, Texture& hdrColorTexture, Texture* bloomTexture, Texture* uiTexture)
{
   SCOPED_LABEL(getName());

   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   hdrColorTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   if (bloomTexture)
   {
      bloomTexture->transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   }
   if (uiTexture)
   {
      uiTexture->transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   }

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [this, &outputTexture, &hdrColorTexture, bloomTexture, uiTexture](vk::CommandBuffer commandBuffer)
   {
      StaticVector<vk::WriteDescriptorSet, 3> descriptorWrites;

      vk::DescriptorImageInfo hdrColorImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(hdrColorTexture.getLayout())
         .setImageView(hdrColorTexture.getDefaultView())
         .setSampler(sampler);
      vk::WriteDescriptorSet hdrColorDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&hdrColorImageInfo);
      descriptorWrites.push(hdrColorDescriptorWrite);

      vk::DescriptorImageInfo bloomImageInfo = vk::DescriptorImageInfo()
         .setSampler(sampler);
      vk::WriteDescriptorSet bloomDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&bloomImageInfo);
      if (bloomTexture)
      {
         bloomImageInfo.setImageLayout(bloomTexture->getLayout());
         bloomImageInfo.setImageView(bloomTexture->getDefaultView());
         descriptorWrites.push(bloomDescriptorWrite);
      }

      vk::DescriptorImageInfo uiImageInfo = vk::DescriptorImageInfo()
         .setSampler(sampler);
      vk::WriteDescriptorSet uiDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(2)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&uiImageInfo);
      if (uiTexture)
      {
         uiImageInfo.setImageLayout(uiTexture->getLayout());
         uiImageInfo.setImageView(uiTexture->getDefaultView());
         descriptorWrites.push(uiDescriptorWrite);
      }

      device.updateDescriptorSets(descriptorWrites, {});

      PipelineDescription<TonemapPass> pipelineDescription;
      pipelineDescription.hdr = outputTexture.getImageProperties().format == vk::Format::eA2R10G10B10UnormPack32;
      pipelineDescription.withBloom = bloomTexture != nullptr;
      pipelineDescription.withUI = uiTexture != nullptr;

      tonemapShader->bindDescriptorSets(commandBuffer, pipelineLayout, descriptorSet);
      renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));
   });
}

Pipeline TonemapPass::createPipeline(const PipelineDescription<TonemapPass>& description, const AttachmentFormats& attachmentFormats)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData(attachmentFormats);
   pipelineData.layout = pipelineLayout;
   pipelineData.shaderStages = tonemapShader->getStages(description.hdr, description.withBloom, description.withUI);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, std::string(description.hdr ? "HDR" : "SDR") + (description.withBloom ? " With Bloom" : " Without Bloom") + (description.withUI ? " With UI" : " Without UI"));

   return pipeline;
}
