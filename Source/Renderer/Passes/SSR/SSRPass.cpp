#include "Renderer/Passes/SSR/SSRPass.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/SceneRenderInfo.h"

#include <utility>

namespace
{
   std::unique_ptr<Texture> createSSRTexture(const GraphicsContext& context, vk::Format format, vk::SampleCountFlagBits sampleCount, uint32_t downscalingFactor)
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
}

SSRPass::SSRPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , generateDescriptorSet(graphicsContext, dynamicDescriptorPool)
{
   NAME_CHILD(generateDescriptorSet, "Generate");
   NAME_CHILD(generatePipelineLayout, "Generate");

   generateShader = createShader<SSRGenerateShader>(context, resourceManager);

   {
      std::array descriptorSetLayouts = generateShader->getDescriptorSetLayouts();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts);
      generatePipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(generatePipelineLayout, "Generate Pipeline Layout");
   }

   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
      .setBorderColor(vk::BorderColor::eFloatTransparentBlack) // TODO correct?
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

SSRPass::~SSRPass()
{
   context.delayedDestroy(std::move(sampler));

   context.delayedDestroy(std::move(generatePipelineLayout));
}

void SSRPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture)
{
   SCOPED_LABEL(getName());

   generate(commandBuffer, sceneRenderInfo, depthTexture, normalTexture);
}

void SSRPass::recreateTextures(vk::SampleCountFlagBits sampleCount)
{
   destroyTextures();
   createTextures(sampleCount);
}

Pipeline SSRPass::createPipeline(const PipelineDescription<SSRPass>& description, const AttachmentFormats& attachmentFormats)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData;
   pipelineData.layout = description.stage == SSRPassStage::Generate ? generatePipelineLayout : generatePipelineLayout; // TODO
   pipelineData.sampleCount = attachmentFormats.sampleCount;
   pipelineData.depthStencilFormat = attachmentFormats.depthStencilFormat;
   pipelineData.colorFormats = attachmentFormats.colorFormats;
   pipelineData.shaderStages = description.stage == SSRPassStage::Generate ? generateShader->getStages() : generateShader->getStages(); // TODO
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.stage == SSRPassStage::Generate ? "Generate" : "?"); // TODO

   return pipeline;
}

void SSRPass::generate(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture)
{
   SCOPED_LABEL("Generate");

   depthTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   normalTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   reflectedUvTexture->transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(*reflectedUvTexture)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }));

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [this, &sceneRenderInfo, &depthTexture, &normalTexture](vk::CommandBuffer commandBuffer)
   {
      vk::DescriptorImageInfo depthBufferImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(depthTexture.getLayout())
         .setImageView(getDepthView(depthTexture))
         .setSampler(sampler);
      vk::WriteDescriptorSet depthBufferDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(generateDescriptorSet.getCurrentSet())
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&depthBufferImageInfo);
      vk::DescriptorImageInfo normalBufferImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(normalTexture.getLayout())
         .setImageView(normalTexture.getDefaultView())
         .setSampler(sampler);
      vk::WriteDescriptorSet normalBufferDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(generateDescriptorSet.getCurrentSet())
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&normalBufferImageInfo);
      device.updateDescriptorSets({ depthBufferDescriptorWrite, normalBufferDescriptorWrite }, {});

      PipelineDescription<SSRPass> pipelineDescription;
      pipelineDescription.stage = SSRPassStage::Generate;

      generateShader->bindDescriptorSets(commandBuffer, generatePipelineLayout, sceneRenderInfo.view.getDescriptorSet(), generateDescriptorSet);
      renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));
   });
}

void SSRPass::createTextures(vk::SampleCountFlagBits sampleCount)
{
   ASSERT(!reflectedUvTexture);
   reflectedUvTexture = createSSRTexture(context, vk::Format::eR16G16B16A16Unorm, sampleCount, 1); // TODO More optimal format?
   NAME_CHILD_POINTER(reflectedUvTexture, "Reflected UV Texture");
}

void SSRPass::destroyTextures()
{
   reflectedUvTexture.reset();
}

vk::ImageView SSRPass::getDepthView(Texture& depthTexture)
{
   bool depthViewCreated = false;
   vk::ImageView depthView = depthTexture.getOrCreateView(vk::ImageViewType::e2D, 0, 1, vk::ImageAspectFlagBits::eDepth, &depthViewCreated);

   if (depthViewCreated)
   {
      NAME_CHILD(depthView, "Depth View");
   }

   return depthView;
}
