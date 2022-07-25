#include "Renderer/Passes/SSAO/SSAOPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/SSAO/SSAOBlurShader.h"
#include "Renderer/Passes/SSAO/SSAOShader.h"
#include "Renderer/SceneRenderInfo.h"

#include <glm/glm.hpp>

#include <random>
#include <utility>

SSAOPass::SSAOPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , ssaoDescriptorSet(graphicsContext, dynamicDescriptorPool, SSAOShader::getLayoutCreateInfo())
   , horizontalBlurDescriptorSet(graphicsContext, dynamicDescriptorPool, SSAOBlurShader::getLayoutCreateInfo())
   , verticalBlurDescriptorSet(graphicsContext, dynamicDescriptorPool, SSAOBlurShader::getLayoutCreateInfo())
   , uniformBuffer(graphicsContext)
{
   NAME_CHILD(ssaoDescriptorSet, "SSAO");
   NAME_CHILD(blurPipelineLayout, "Blur");
   NAME_CHILD(uniformBuffer, "");

   ssaoShader = std::make_unique<SSAOShader>(context, resourceManager);
   blurShader = std::make_unique<SSAOBlurShader>(context, resourceManager);

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = ssaoShader->getSetLayouts();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts);
      ssaoPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(ssaoPipelineLayout, "SSAO Pipeline Layout");
   }

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = blurShader->getSetLayouts();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts);
      blurPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(blurPipelineLayout, "Blur Pipeline Layout");
   }

   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eNearest)
      .setMinFilter(vk::Filter::eNearest)
      .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
      .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
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

   {
      SSAOUniformData uniformData;

      std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
      std::default_random_engine generator;

      for (uint32_t i = 0; i < uniformData.samples.size(); ++i)
      {
         glm::vec4& sample = uniformData.samples[i];
         sample = glm::vec4(glm::normalize(glm::vec3(distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator))), 0.0f);
         sample *= distribution(generator);

         float scale = static_cast<float>(i) / uniformData.samples.size();
         scale = glm::mix(0.1f, 1.0f, scale * scale);
         sample *= scale;
      }

      for (glm::vec4& noiseValue : uniformData.noise)
      {
         noiseValue = glm::vec4(distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f, distribution(generator) * 2.0f - 1.0f);
      }

      uniformBuffer.updateAll(uniformData);

      for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
      {
         vk::DescriptorBufferInfo bufferInfo = uniformBuffer.getDescriptorBufferInfo(frameIndex);

         vk::WriteDescriptorSet bufferDescriptorWrite = vk::WriteDescriptorSet()
            .setDstSet(ssaoDescriptorSet.getSet(frameIndex))
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&bufferInfo);

         device.updateDescriptorSets({ bufferDescriptorWrite }, {});
      }
   }
}

SSAOPass::~SSAOPass()
{
   context.delayedDestroy(std::move(sampler));

   context.delayedDestroy(std::move(ssaoPipelineLayout));
   context.delayedDestroy(std::move(blurPipelineLayout));

   ssaoShader.reset();
   blurShader.reset();
}

void SSAOPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture, Texture& ssaoTexture, Texture& ssaoBlurTexture)
{
   SCOPED_LABEL(getName());

   renderSSAO(commandBuffer, sceneRenderInfo, depthTexture, normalTexture, ssaoTexture);

   renderBlur(commandBuffer, sceneRenderInfo, depthTexture, ssaoTexture, ssaoBlurTexture, true);
   renderBlur(commandBuffer, sceneRenderInfo, depthTexture, ssaoBlurTexture, ssaoTexture, false);
}

Pipeline SSAOPass::createPipeline(const PipelineDescription<SSAOPass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Screen;

   PipelineData pipelineData;
   pipelineData.layout = description.blur ? blurPipelineLayout : ssaoPipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.depthStencilFormat = getDepthStencilFormat();
   pipelineData.colorFormats = getColorFormats();
   pipelineData.shaderStages = description.blur ? blurShader->getStages(description.horizontal) : ssaoShader->getStages();
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.blur ? (description.horizontal ? "Blur (Horizontal)" : "Blur (Vertical)") : "SSAO");

   return pipeline;
}

void SSAOPass::renderSSAO(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture, Texture& ssaoTexture)
{
   SCOPED_LABEL("SSAO");

   depthTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   normalTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   ssaoTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   vk::RenderingAttachmentInfo colorAttachmentInfo = vk::RenderingAttachmentInfo()
      .setImageView(ssaoTexture.getDefaultView())
      .setImageLayout(ssaoTexture.getLayout())
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   beginRenderPass(commandBuffer, ssaoTexture.getExtent(), nullptr, &colorAttachmentInfo);

   vk::DescriptorImageInfo depthBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(depthTexture.getLayout())
      .setImageView(getDepthView(depthTexture))
      .setSampler(sampler);
   vk::WriteDescriptorSet depthBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(ssaoDescriptorSet.getCurrentSet())
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
      .setDstSet(ssaoDescriptorSet.getCurrentSet())
      .setDstBinding(1)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&normalBufferImageInfo);
   device.updateDescriptorSets({ depthBufferDescriptorWrite, normalBufferDescriptorWrite }, {});

   PipelineDescription<SSAOPass> pipelineDescription;
   pipelineDescription.blur = false;

   ssaoShader->bindDescriptorSets(commandBuffer, ssaoPipelineLayout, sceneRenderInfo.view, ssaoDescriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}

void SSAOPass::renderBlur(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& inputTexture, Texture& outputTexture, bool horizontal)
{
   SCOPED_LABEL(std::string(horizontal ? "Horizontal" : "Vertical") + " Blur");

   depthTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   inputTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   vk::RenderingAttachmentInfo colorAttachmentInfo = vk::RenderingAttachmentInfo()
      .setImageView(outputTexture.getDefaultView())
      .setImageLayout(outputTexture.getLayout())
      .setLoadOp(vk::AttachmentLoadOp::eDontCare);

   beginRenderPass(commandBuffer, outputTexture.getExtent(), nullptr, &colorAttachmentInfo);

   DescriptorSet& blurDescriptorSet = horizontal ? horizontalBlurDescriptorSet : verticalBlurDescriptorSet;

   vk::DescriptorImageInfo sourceBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(inputTexture.getLayout())
      .setImageView(inputTexture.getDefaultView())
      .setSampler(sampler);
   vk::WriteDescriptorSet sourceBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(blurDescriptorSet.getCurrentSet())
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&sourceBufferImageInfo);
   vk::DescriptorImageInfo depthBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(depthTexture.getLayout())
      .setImageView(getDepthView(depthTexture))
      .setSampler(sampler);
   vk::WriteDescriptorSet depthBufferDescriptorWrite = vk::WriteDescriptorSet()
      .setDstSet(blurDescriptorSet.getCurrentSet())
      .setDstBinding(1)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setPImageInfo(&depthBufferImageInfo);
   device.updateDescriptorSets({ sourceBufferDescriptorWrite, depthBufferDescriptorWrite }, {});

   PipelineDescription<SSAOPass> pipelineDescription;
   pipelineDescription.blur = true;
   pipelineDescription.horizontal = horizontal;

   blurShader->bindDescriptorSets(commandBuffer, blurPipelineLayout, sceneRenderInfo.view, blurDescriptorSet);
   renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));

   endRenderPass(commandBuffer);
}

vk::ImageView SSAOPass::getDepthView(Texture& depthTexture)
{
   bool depthViewCreated = false;
   vk::ImageView depthView = depthTexture.getOrCreateView(vk::ImageViewType::e2D, 0, 1, vk::ImageAspectFlagBits::eDepth, &depthViewCreated);

   if (depthViewCreated)
   {
      NAME_CHILD(depthView, "Depth View");
   }

   return depthView;
}
