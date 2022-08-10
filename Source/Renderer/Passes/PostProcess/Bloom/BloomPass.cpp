#include "Renderer/Passes/PostProcess/Bloom/BloomPass.h"

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

#if FORGE_WITH_DEBUG_UTILS
   std::string getTextureResolutionString(const Texture& texture)
   {
      const ImageProperties& imageProperties = texture.getImageProperties();
      return DebugUtils::toString(imageProperties.width) + "x" + DebugUtils::toString(imageProperties.height);
   }
#endif // FORGE_WITH_DEBUG_UTILS
}

BloomPass::BloomPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , downsampleShader(std::make_unique<BloomDownsampleShader>(context, resourceManager))
   , upsampleShader(std::make_unique<BloomUpsampleShader>(context, resourceManager))
{
   downsampleDescriptorSets.reserve(kNumSteps);
   upsampleDescriptorSets.reserve(kNumSteps);
   upsampleUniformBuffers.reserve(kNumSteps);
   for (uint32_t i = 0; i < kNumSteps; ++i)
   {
      downsampleDescriptorSets.emplace_back(context, dynamicDescriptorPool, BloomDownsampleShader::getLayoutCreateInfo());
      upsampleDescriptorSets.emplace_back(context, dynamicDescriptorPool, BloomUpsampleShader::getLayoutCreateInfo());
      upsampleUniformBuffers.emplace_back(context);

      NAME_CHILD(downsampleDescriptorSets.back(), "Downsample Step " + DebugUtils::toString(i));
      NAME_CHILD(upsampleDescriptorSets.back(), "Upsample Step " + DebugUtils::toString(i));
      NAME_CHILD(upsampleUniformBuffers.back(), "Step " + DebugUtils::toString(i));

      for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
      {
         vk::DescriptorBufferInfo bufferInfo = upsampleUniformBuffers[i].getDescriptorBufferInfo(frameIndex);

         vk::WriteDescriptorSet bufferDescriptorWrite = vk::WriteDescriptorSet()
            .setDstSet(upsampleDescriptorSets[i].getSet(frameIndex))
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&bufferInfo);

         device.updateDescriptorSets({ bufferDescriptorWrite }, {});
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

void BloomPass::render(vk::CommandBuffer commandBuffer, Texture& hdrColorTexture, Texture& defaultBlackTexture)
{
   ASSERT(hdrColorTexture.getExtent() == context.getSwapchain().getExtent());

   SCOPED_LABEL(getName());

   {
      SCOPED_LABEL("Downsample");
      for (uint32_t step = 0; step < kNumSteps; ++step)
      {
         renderDownsample(commandBuffer, step, hdrColorTexture);
      }
   }

   {
      SCOPED_LABEL("Upsample");
      for (uint32_t step = kNumSteps; step > 0; --step)
      {
         renderUpsample(commandBuffer, step - 1, defaultBlackTexture);
      }
   }
}

void BloomPass::postUpdateAttachmentFormats()
{
   SceneRenderPass::postUpdateAttachmentFormats();

   destroyTextures();
   createTextures();
}

Pipeline BloomPass::createPipeline(const PipelineDescription<BloomPass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData;
   pipelineData.layout = description.upsample ? upsamplePipelineLayout : downsamplePipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.depthStencilFormat = getDepthStencilFormat();
   pipelineData.colorFormats = getColorFormats();
   pipelineData.shaderStages = description.upsample ? upsampleShader->getStages() : downsampleShader->getStages();
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.upsample ? "Upsample" : "Downsample");

   return pipeline;
}

void BloomPass::renderDownsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& hdrColorTexture)
{
   ASSERT(step < kNumSteps);

   Texture& inputTexture = step == 0 ? hdrColorTexture : *downsampleTextures[step - 1];
   Texture& outputTexture = *downsampleTextures[step];
   const DescriptorSet& descriptorSet = downsampleDescriptorSets[step];

   SCOPED_LABEL(getTextureResolutionString(inputTexture) + " --> " + getTextureResolutionString(outputTexture));

   inputTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   beginRenderPass(commandBuffer, &colorAttachmentInfo);

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
   pipelineDescription.upsample = false;

   downsampleShader->bindDescriptorSets(commandBuffer, downsamplePipelineLayout, descriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}

void BloomPass::renderUpsample(vk::CommandBuffer commandBuffer, uint32_t step, Texture& defaultBlackTexture)
{
   ASSERT(step < kNumSteps);

   Texture& downsampleTexture = *downsampleTextures[step];
   Texture& previousUpsampleTexture = step == kNumSteps - 1 ? defaultBlackTexture : *upsampleTextures[step + 1];
   Texture& outputTexture = *upsampleTextures[step];
   const DescriptorSet& descriptorSet = upsampleDescriptorSets[step];
   UniformBuffer<BloomUpsampleUniformData>& uniformBuffer = upsampleUniformBuffers[step];

   SCOPED_LABEL(getTextureResolutionString(outputTexture));

   BloomUpsampleUniformData uniformData;
   uniformData.filterRadius = 1.0f + step * 0.35f;
   uniformData.colorMix = step == kNumSteps - 1 ? 0.0f : 0.5f;
   uniformBuffer.update(uniformData);

   downsampleTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   previousUpsampleTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   beginRenderPass(commandBuffer, &colorAttachmentInfo);

   vk::DescriptorImageInfo downsampleImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(downsampleTexture.getLayout())
      .setImageView(downsampleTexture.getDefaultView())
      .setSampler(sampler);
   vk::DescriptorImageInfo previousUpsampleImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(previousUpsampleTexture.getLayout())
      .setImageView(previousUpsampleTexture.getDefaultView())
      .setSampler(sampler);
   vk::WriteDescriptorSet downsampleDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(descriptorSet.getCurrentSet())
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&downsampleImageInfo);
   vk::WriteDescriptorSet previousUpsampleDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(descriptorSet.getCurrentSet())
      .setDstBinding(1)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&previousUpsampleImageInfo);
   device.updateDescriptorSets({ downsampleDescriptorWrite, previousUpsampleDescriptorWrite }, {});

   PipelineDescription<BloomPass> pipelineDescription;
   pipelineDescription.upsample = true;

   upsampleShader->bindDescriptorSets(commandBuffer, upsamplePipelineLayout, descriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}

void BloomPass::createTextures()
{
   ASSERT(getColorFormats().size() == 1);
   vk::Format format = getColorFormats()[0];

   for (uint32_t i = 0; i < kNumSteps; ++i)
   {
      ASSERT(!downsampleTextures[i]);
      ASSERT(!upsampleTextures[i]);

      downsampleTextures[i] = createBloomTexture(context, format, getSampleCount(), 1 << (i + 1));
      upsampleTextures[i] = createBloomTexture(context, format, getSampleCount(), 1 << (i + 1));

      NAME_CHILD_POINTER(downsampleTextures[i], "Downsample Texture " + DebugUtils::toString(i));
      NAME_CHILD_POINTER(upsampleTextures[i], "Upsample Texture " + DebugUtils::toString(i));
   }
}

void BloomPass::destroyTextures()
{
   for (std::unique_ptr<Texture>& downsampleTexture : downsampleTextures)
   {
      downsampleTexture.reset();
   }
   for (std::unique_ptr<Texture>& upsampleTexture : upsampleTextures)
   {
      upsampleTexture.reset();
   }
}
