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
   , descriptorSet(graphicsContext, dynamicDescriptorPool)
{
   compositeShader = createShader<CompositeShader>(context, resourceManager);

   std::array descriptorSetLayouts = compositeShader->getDescriptorSetLayouts();
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
}

void CompositePass::render(vk::CommandBuffer commandBuffer, Texture& destinationTexture, Texture& sourceTexture, CompositeShader::Mode mode)
{
   SCOPED_LABEL(getName());

   destinationTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   sourceTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);

   AttachmentInfo colorAttachmentInfo(destinationTexture);
   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [this, &sourceTexture, mode](vk::CommandBuffer commandBuffer)
   {
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
   });
}

Pipeline CompositePass::createPipeline(const PipelineDescription<CompositePass>& description, const AttachmentFormats& attachmentFormats)
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

   PipelineData pipelineData(attachmentFormats);
   pipelineData.layout = pipelineLayout;
   pipelineData.shaderStages = compositeShader->getStages(description.mode);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, getModeName(description.mode));

   return pipeline;
}
