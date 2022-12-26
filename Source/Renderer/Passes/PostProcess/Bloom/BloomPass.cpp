#include "Renderer/Passes/PostProcess/Bloom/BloomPass.h"

#include "Core/Assert.h"
#include "Core/Enum.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"
#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"

#include <string>
#include <utility>

namespace
{
   std::unique_ptr<Texture> createBloomTexture(const GraphicsContext& context, vk::Format format, vk::SampleCountFlagBits sampleCount, uint32_t downscalingFactor)
   {
      ASSERT(downscalingFactor > 0);

      const Swapchain& swapchain = context.getSwapchain();

      ImageProperties imageProperties;
      imageProperties.format = format;
      imageProperties.width = swapchain.getExtent().width / downscalingFactor;
      imageProperties.height = swapchain.getExtent().height / downscalingFactor;

      TextureProperties textureProperties;
      textureProperties.sampleCount = sampleCount;
      textureProperties.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
      textureProperties.aspects = vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout initialLayout;
      initialLayout.layout = vk::ImageLayout::eColorAttachmentOptimal;
      initialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

      return std::make_unique<Texture>(context, imageProperties, textureProperties, initialLayout);
   }

   RenderQuality getDownsampleStepQuality(RenderQuality overallQuality, uint32_t step)
   {
      switch (overallQuality)
      {
      case RenderQuality::Disabled:
         return RenderQuality::Disabled;
      case RenderQuality::Low:
         // Low quality for the higher resolutions, medium quality for the lower resolutions
         return step <= BloomPass::kNumSteps / 2 ? RenderQuality::Low : RenderQuality::Medium;
      case RenderQuality::Medium:
         // Low quality for the highest resolution, medium quality for the rest
         return step == 0 ? RenderQuality::Low : RenderQuality::Medium;
      case RenderQuality::High:
         // Medium quality for the highest resolution, high quality for the rest
         return step == 0 ? RenderQuality::Medium : RenderQuality::High;
      default:
         ASSERT(false);
         return RenderQuality::Disabled;
      }
   }

   RenderQuality getUpsampleStepQuality(RenderQuality overallQuality, uint32_t step)
   {
      switch (overallQuality)
      {
      case RenderQuality::Disabled:
         return RenderQuality::Disabled;
      case RenderQuality::Low:
         // Low quality for the higher resolutions, medium quality for the lower resolutions
         return step <= BloomPass::kNumSteps / 2 ? RenderQuality::Low : RenderQuality::Medium;
      case RenderQuality::Medium:
         // Medium quality for the higher resolutions, high quality for the lower resolutions
         return step <= BloomPass::kNumSteps / 2 ? RenderQuality::Medium : RenderQuality::High;
      case RenderQuality::High:
         // Always high quality
         return RenderQuality::High;
      default:
         ASSERT(false);
         return RenderQuality::Disabled;
      }
   }

#if FORGE_WITH_DEBUG_UTILS
   std::string getTextureResolutionString(const Texture& texture)
   {
      const ImageProperties& imageProperties = texture.getImageProperties();
      return DebugUtils::toString(imageProperties.width) + "x" + DebugUtils::toString(imageProperties.height);
   }

   const char* getBloomPassTypeString(BloomPassType type)
   {
      switch (type)
      {
      case BloomPassType::Downsample:
         return "Downsample";
      case BloomPassType::HorizontalUpsample:
         return "Horizontal Upsample";
      case BloomPassType::VerticalUpsample:
         return "Vertical Upsample";
      default:
         ASSERT(false);
         return nullptr;
      }
   }
#endif // FORGE_WITH_DEBUG_UTILS
}

BloomPass::BloomPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , downsampleShader(std::make_unique<BloomDownsampleShader>(context, resourceManager))
   , upsampleShader(std::make_unique<BloomUpsampleShader>(context, resourceManager))
{
   downsampleDescriptorSets.reserve(kNumSteps);
   horizontalUpsampleDescriptorSets.reserve(kNumSteps);
   verticalUpsampleDescriptorSets.reserve(kNumSteps);
   upsampleUniformBuffers.reserve(kNumSteps);
   for (uint32_t i = 0; i < kNumSteps; ++i)
   {
      downsampleDescriptorSets.emplace_back(context, dynamicDescriptorPool, BloomDownsampleShader::getLayoutCreateInfo());
      horizontalUpsampleDescriptorSets.emplace_back(context, dynamicDescriptorPool, BloomUpsampleShader::getLayoutCreateInfo());
      verticalUpsampleDescriptorSets.emplace_back(context, dynamicDescriptorPool, BloomUpsampleShader::getLayoutCreateInfo());
      upsampleUniformBuffers.emplace_back(context);

      NAME_CHILD(downsampleDescriptorSets.back(), "Downsample Step " + DebugUtils::toString(i));
      NAME_CHILD(horizontalUpsampleDescriptorSets.back(), "Horizontal Upsample Step " + DebugUtils::toString(i));
      NAME_CHILD(verticalUpsampleDescriptorSets.back(), "Vertical Upsample Step " + DebugUtils::toString(i));
      NAME_CHILD(upsampleUniformBuffers.back(), "Step " + DebugUtils::toString(i));

      for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
      {
         vk::DescriptorBufferInfo bufferInfo = upsampleUniformBuffers[i].getDescriptorBufferInfo(frameIndex);

         vk::WriteDescriptorSet horizontalBufferDescriptorWrite = vk::WriteDescriptorSet()
            .setDstSet(horizontalUpsampleDescriptorSets[i].getSet(frameIndex))
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&bufferInfo);

         vk::WriteDescriptorSet verticalBufferDescriptorWrite = vk::WriteDescriptorSet()
            .setDstSet(verticalUpsampleDescriptorSets[i].getSet(frameIndex))
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&bufferInfo);

         device.updateDescriptorSets({ horizontalBufferDescriptorWrite, verticalBufferDescriptorWrite }, {});
      }
   }

   std::vector<vk::DescriptorSetLayout> downsampleDescriptorSetLayouts = downsampleShader->getSetLayouts();
   vk::PipelineLayoutCreateInfo downsamplePipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(downsampleDescriptorSetLayouts);
   downsamplePipelineLayout = device.createPipelineLayout(downsamplePipelineLayoutCreateInfo);
   NAME_CHILD(downsamplePipelineLayout, "Downsample Pipeline Layout");

   std::vector<vk::DescriptorSetLayout> upsampleDescriptorSetLayouts = upsampleShader->getSetLayouts();
   vk::PipelineLayoutCreateInfo upsamplePipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(upsampleDescriptorSetLayouts);
   upsamplePipelineLayout = device.createPipelineLayout(upsamplePipelineLayoutCreateInfo);
   NAME_CHILD(upsamplePipelineLayout, "Upsample Pipeline Layout");

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

BloomPass::~BloomPass()
{
   destroyTextures();

   context.delayedDestroy(std::move(sampler));
   context.delayedDestroy(std::move(downsamplePipelineLayout));
   context.delayedDestroy(std::move(upsamplePipelineLayout));

   downsampleShader.reset();
   upsampleShader.reset();
}

void BloomPass::render(vk::CommandBuffer commandBuffer, Texture& hdrColorTexture, Texture& defaultBlackTexture, RenderQuality quality)
{
   ASSERT(hdrColorTexture.getExtent() == context.getSwapchain().getExtent());

   SCOPED_LABEL(getName());

   {
      SCOPED_LABEL("Downsample");
      for (uint32_t step = 0; step < kNumSteps; ++step)
      {
         renderDownsample(commandBuffer, step, hdrColorTexture, quality);
      }
   }

   {
      SCOPED_LABEL("Upsample");
      for (uint32_t step = kNumSteps; step > 0; --step)
      {
         renderUpsample(commandBuffer, step - 1, defaultBlackTexture, quality, true);
         renderUpsample(commandBuffer, step - 1, defaultBlackTexture, quality, false);
      }
   }
}

void BloomPass::recreateTextures(vk::Format format, vk::SampleCountFlagBits sampleCount)
{
   destroyTextures();
   createTextures(format, sampleCount);
}

Pipeline BloomPass::createPipeline(const PipelineDescription<BloomPass>& description, const AttachmentFormats& attachmentFormats)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData(attachmentFormats);
   pipelineData.layout = description.type == BloomPassType::Downsample ? downsamplePipelineLayout : upsamplePipelineLayout;
   pipelineData.shaderStages = description.type == BloomPassType::Downsample ? downsampleShader->getStages(description.quality) : upsampleShader->getStages(description.quality, description.type == BloomPassType::HorizontalUpsample);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, getBloomPassTypeString(description.type));

   return pipeline;
}

void BloomPass::renderDownsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& hdrColorTexture, RenderQuality quality)
{
   ASSERT(step < kNumSteps);

   Texture& inputTexture = step == 0 ? hdrColorTexture : *textures[step - 1];
   Texture& outputTexture = *textures[step];
   const DescriptorSet& descriptorSet = downsampleDescriptorSets[step];

   RenderQuality stepQuality = getDownsampleStepQuality(quality, step);

   SCOPED_LABEL(getTextureResolutionString(inputTexture) + " --> " + getTextureResolutionString(outputTexture) + " (" + RenderSettings::getQualityString(stepQuality) + ")");

   inputTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [this, &inputTexture, &descriptorSet, stepQuality](vk::CommandBuffer commandBuffer)
   {
      vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
         .setImageLayout(inputTexture.getLayout())
         .setImageView(inputTexture.getDefaultView())
         .setSampler(sampler);
      vk::WriteDescriptorSet descriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo);
      device.updateDescriptorSets(descriptorWrite, {});

      PipelineDescription<BloomPass> pipelineDescription;
      pipelineDescription.type = BloomPassType::Downsample;
      pipelineDescription.quality = stepQuality;

      downsampleShader->bindDescriptorSets(commandBuffer, downsamplePipelineLayout, descriptorSet);
      renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));
   });
}

void BloomPass::renderUpsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& defaultBlackTexture, RenderQuality quality, bool horizontal)
{
   ASSERT(step < kNumSteps);

   Texture& inputTexture = horizontal ? *textures[step] : *horizontalBlurTextures[step];
   Texture& blendTexture = step == kNumSteps - 1 ? defaultBlackTexture : *textures[step + 1];
   Texture& outputTexture = horizontal ? *horizontalBlurTextures[step] : *textures[step];
   const DescriptorSet& descriptorSet = horizontal ? horizontalUpsampleDescriptorSets[step] : verticalUpsampleDescriptorSets[step];
   UniformBuffer<BloomUpsampleUniformData>& uniformBuffer = upsampleUniformBuffers[step];

   RenderQuality stepQuality = getUpsampleStepQuality(quality, step);

   SCOPED_LABEL(getTextureResolutionString(outputTexture) + (horizontal ? " Horizontal" : " Vertical") + (horizontal || step == kNumSteps - 1 ? "" : " + " + getTextureResolutionString(blendTexture)) + " (" + RenderSettings::getQualityString(stepQuality) + ")");

   BloomUpsampleUniformData uniformData;
   uniformData.filterRadius = 1.0f + step * 0.35f;
   uniformData.colorMix = step == kNumSteps - 1 ? 0.0f : 0.5f;
   uniformBuffer.update(uniformData);

   inputTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   blendTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [this, horizontal, &inputTexture, &blendTexture, &descriptorSet, stepQuality](vk::CommandBuffer commandBuffer)
   {
      vk::DescriptorImageInfo inputImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(inputTexture.getLayout())
         .setImageView(inputTexture.getDefaultView())
         .setSampler(sampler);
      vk::DescriptorImageInfo blendImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(blendTexture.getLayout())
         .setImageView(blendTexture.getDefaultView())
         .setSampler(sampler);
      vk::WriteDescriptorSet inputDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&inputImageInfo);
      vk::WriteDescriptorSet blendDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getCurrentSet())
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&blendImageInfo);
      device.updateDescriptorSets({ inputDescriptorWrite, blendDescriptorWrite }, {});

      PipelineDescription<BloomPass> pipelineDescription;
      pipelineDescription.type = horizontal ? BloomPassType::HorizontalUpsample : BloomPassType::VerticalUpsample;
      pipelineDescription.quality = stepQuality;

      upsampleShader->bindDescriptorSets(commandBuffer, upsamplePipelineLayout, descriptorSet);
      renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));
   });
}

void BloomPass::createTextures(vk::Format format, vk::SampleCountFlagBits sampleCount)
{
   for (uint32_t i = 0; i < kNumSteps; ++i)
   {
      ASSERT(!textures[i]);
      ASSERT(!horizontalBlurTextures[i]);

      textures[i] = createBloomTexture(context, format, sampleCount, 1 << (i + 1));
      horizontalBlurTextures[i] = createBloomTexture(context, format, sampleCount, 1 << (i + 1));

      NAME_CHILD_POINTER(textures[i], "Texture " + DebugUtils::toString(i));
      NAME_CHILD_POINTER(horizontalBlurTextures[i], "Horizontal Blur Texture " + DebugUtils::toString(i));
   }
}

void BloomPass::destroyTextures()
{
   for (std::unique_ptr<Texture>& texture : textures)
   {
      texture.reset();
   }
   for (std::unique_ptr<Texture>& horizontalBlurTexture : horizontalBlurTextures)
   {
      horizontalBlurTexture.reset();
   }
}
