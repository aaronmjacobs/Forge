#include "Renderer/Passes/PostProcess/Tonemap/TonemapPass.h"

#include "Core/Containers/StaticVector.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Resources/ResourceManager.h"

#include <utility>

namespace
{
   ColorGamut getColorGamut(vk::ColorSpaceKHR colorSpace)
   {
      switch (colorSpace)
      {
      case vk::ColorSpaceKHR::eSrgbNonlinear:
      case vk::ColorSpaceKHR::eExtendedSrgbLinearEXT:
      case vk::ColorSpaceKHR::eBt709LinearEXT:
      case vk::ColorSpaceKHR::eBt709NonlinearEXT:
      case vk::ColorSpaceKHR::eExtendedSrgbNonlinearEXT:
         return ColorGamut::Rec709;
      case vk::ColorSpaceKHR::eBt2020LinearEXT:
      case vk::ColorSpaceKHR::eHdr10St2084EXT:
      case vk::ColorSpaceKHR::eHdr10HlgEXT:
         return ColorGamut::Rec2020;
      case vk::ColorSpaceKHR::eDisplayP3NonlinearEXT:
      case vk::ColorSpaceKHR::eDisplayP3LinearEXT:
         return ColorGamut::P3;
      default:
         // Don't support anything else
         return ColorGamut::Rec709;
      }
   }

   TransferFunction getTransferFunction(vk::ColorSpaceKHR colorSpace)
   {
      switch (colorSpace)
      {
      case vk::ColorSpaceKHR::eSrgbNonlinear: // Handled in hardware, so we treat it as linear
      case vk::ColorSpaceKHR::eExtendedSrgbLinearEXT:
      case vk::ColorSpaceKHR::eDisplayP3LinearEXT:
      case vk::ColorSpaceKHR::eBt709LinearEXT:
      case vk::ColorSpaceKHR::eBt2020LinearEXT:
      case vk::ColorSpaceKHR::eAdobergbLinearEXT:
      case vk::ColorSpaceKHR::ePassThroughEXT:
         return TransferFunction::Linear;
      case vk::ColorSpaceKHR::eDisplayP3NonlinearEXT:
      case vk::ColorSpaceKHR::eDciP3NonlinearEXT:
      case vk::ColorSpaceKHR::eBt709NonlinearEXT: // Technically uses a gamma of 2.4, but we don't support that, so this is the closest option
      case vk::ColorSpaceKHR::eAdobergbNonlinearEXT: // Technically uses a gamma of ~2.2, but we don't support that, so this is the closest option
      case vk::ColorSpaceKHR::eExtendedSrgbNonlinearEXT:
         return TransferFunction::sRGB;
      case vk::ColorSpaceKHR::eHdr10St2084EXT:
         return TransferFunction::PerceptualQuantizer;
      case vk::ColorSpaceKHR::eHdr10HlgEXT:
         return TransferFunction::HybridLogGamma;
      default:
         // Don't support anything else
         return TransferFunction::Linear;
      }
   }
}

TonemapPass::TonemapPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool)
   , uniformBuffer(graphicsContext)
{
   NAME_CHILD(uniformBuffer, "");

   tonemapShader = createShader<TonemapShader>(context, resourceManager);

   std::array descriptorSetLayouts = tonemapShader->getDescriptorSetLayouts();
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

   TextureLoadOptions lutLoadOptions;
   lutLoadOptions.sRGB = false;
   lutLoadOptions.generateMipMaps = false;
   lutLoadOptions.fallbackDefaultTextureType = DefaultTextureType::Volume;
   lutTextureHandle = resourceManager.loadTexture("Resources/Textures/Tony McMapface/tony_mc_mapface.dds", lutLoadOptions);

   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo bufferInfo = uniformBuffer.getDescriptorBufferInfo(frameIndex);

      vk::WriteDescriptorSet bufferDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(4)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&bufferInfo);

      device.updateDescriptorSets({ bufferDescriptorWrite }, {});
   }
}

TonemapPass::~TonemapPass()
{
   context.delayedDestroy(std::move(sampler));
   context.delayedDestroy(std::move(pipelineLayout));
}

void TonemapPass::render(vk::CommandBuffer commandBuffer, Texture& outputTexture, Texture& hdrColorTexture, Texture* bloomTexture, Texture* uiTexture, const TonemapSettings& settings, vk::ColorSpaceKHR outputColorSpace)
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

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [this, &outputTexture, &hdrColorTexture, bloomTexture, uiTexture, settings, outputColorSpace](vk::CommandBuffer commandBuffer)
   {
      StaticVector<vk::WriteDescriptorSet, 4> descriptorWrites;

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

      vk::DescriptorImageInfo lutImageInfo = vk::DescriptorImageInfo()
         .setSampler(sampler);
      vk::WriteDescriptorSet lutDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(3)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&lutImageInfo);
      if (Texture* lutTexture = lutTextureHandle.getResource())
      {
         lutImageInfo.setImageLayout(lutTexture->getLayout());
         lutImageInfo.setImageView(lutTexture->getDefaultView());
         descriptorWrites.push(lutDescriptorWrite);
      }

      device.updateDescriptorSets(descriptorWrites, {});

      TonemapUniformData uniformData;
      uniformData.bloomStrength = settings.bloomStrength;
      uniformData.paperWhiteNits = settings.paperWhiteNits;
      uniformData.peakBrightnessNits = settings.peakBrightnessNits;
      uniformData.toe = settings.toe;
      uniformData.shoulder = settings.shoulder;
      uniformData.hotspot = settings.hotspot;
      uniformData.huePreservation = settings.huePreservation;
      uniformBuffer.update(uniformData);

      PipelineDescription<TonemapPass> pipelineDescription;
      TonemapShaderConstants& shaderConstants = pipelineDescription.shaderConstants;
      shaderConstants.tonemappingAlgorithm = settings.algorithm;
      shaderConstants.colorGamut = settings.convertToOutputColorGamut ? getColorGamut(outputColorSpace) : ColorGamut::Rec709;
      shaderConstants.transferFunction = getTransferFunction(outputColorSpace);
      shaderConstants.outputHDR = FormatHelpers::isUsedForHdrPresentation(outputTexture.getImageProperties().format);
      shaderConstants.withBloom = bloomTexture != nullptr;
      shaderConstants.withUI = uiTexture != nullptr;
      shaderConstants.showTestPattern = settings.showTestPattern;

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
   pipelineData.shaderStages = tonemapShader->getStages(description.shaderConstants);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, std::string(description.shaderConstants.outputHDR ? "HDR" : "SDR") + (description.shaderConstants.withBloom ? " With Bloom" : " Without Bloom") + (description.shaderConstants.withUI ? " With UI" : " Without UI") + (description.shaderConstants.showTestPattern ? " With Test Pattern" : " Without Test Pattern"));

   return pipeline;
}
