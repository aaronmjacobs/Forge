#include "Renderer/Passes/Forward/ForwardPass.h"

#include "Core/Containers/StaticVector.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/Passes/Forward/ForwardShader.h"
#include "Renderer/Passes/Forward/SkyboxShader.h"
#include "Renderer/SceneRenderInfo.h"

#include <array>
#include <utility>

ForwardPass::ForwardPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager, const ForwardLighting* forwardLighting)
   : SceneRenderPass(graphicsContext)
   , forwardDescriptorSet(graphicsContext, dynamicDescriptorPool, ForwardShader::getLayoutCreateInfo())
   , skyboxDescriptorSet(graphicsContext, dynamicDescriptorPool, SkyboxShader::getLayoutCreateInfo())
   , lighting(forwardLighting)
{
   forwardShader = createShader<ForwardShader>(context, resourceManager);
   skyboxShader = createShader<SkyboxShader>(context, resourceManager);

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = forwardShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = forwardShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      forwardPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(forwardPipelineLayout, "Forward Pipeline Layout");
   }

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = skyboxShader->getSetLayouts();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts);
      skyboxPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(skyboxPipelineLayout, "Skybox Pipeline Layout");
   }

   vk::SamplerCreateInfo skyboxSamplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
      .setAnisotropyEnable(false)
      .setMaxAnisotropy(1.0f)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(0.0f)
      .setMinLod(0.0f)
      .setMaxLod(16.0f);
   skyboxSampler = context.getDevice().createSampler(skyboxSamplerCreateInfo);
   NAME_CHILD(skyboxSampler, "Skybox Sampler");

   vk::SamplerCreateInfo normalSamplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eNearest)
      .setMinFilter(vk::Filter::eNearest)
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
   normalSampler = context.getDevice().createSampler(normalSamplerCreateInfo);
   NAME_CHILD(normalSampler, "Normal Sampler");
}

ForwardPass::~ForwardPass()
{
   context.delayedDestroy(std::move(normalSampler));
   context.delayedDestroy(std::move(skyboxSampler));

   context.delayedDestroy(std::move(forwardPipelineLayout));
   context.delayedDestroy(std::move(skyboxPipelineLayout));
}

void ForwardPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& colorTexture, Texture* colorResolveTexture, Texture& roughnessMetalnessTexture, Texture& normalTexture, Texture& ssaoTexture, const Texture* skyboxTexture)
{
   SCOPED_LABEL(getName());

   depthTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   colorTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   roughnessMetalnessTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   if (colorResolveTexture)
   {
      colorResolveTexture->transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   }
   normalTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   ssaoTexture.transitionLayout(commandBuffer, TextureLayoutType::ShaderRead);
   ASSERT(!skyboxTexture || skyboxTexture->getLayout() == vk::ImageLayout::eShaderReadOnlyOptimal);

   std::array<AttachmentInfo,2 > colorAttachmentInfo;
   colorAttachmentInfo[0] = AttachmentInfo(colorTexture)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearColorValue(std::array<float, 4>{ 10.0f, 10.0f, 10.0f, 1.0f }));
   if (colorResolveTexture)
   {
      colorAttachmentInfo[0].setResolveTexture(*colorResolveTexture);
      colorAttachmentInfo[0].setResolveMode(vk::ResolveModeFlagBits::eAverage);
   }
   colorAttachmentInfo[1] = AttachmentInfo(roughnessMetalnessTexture)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearColorValue(std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 0.0f }));

   AttachmentInfo depthStencilAttachmentInfo(depthTexture);

   executePass(commandBuffer, colorAttachmentInfo, &depthStencilAttachmentInfo, [this, &sceneRenderInfo, &normalTexture, &ssaoTexture, skyboxTexture](vk::CommandBuffer commandBuffer)
   {
      vk::DescriptorImageInfo normalBufferImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(normalTexture.getLayout())
         .setImageView(normalTexture.getDefaultView())
         .setSampler(normalSampler);
      vk::DescriptorImageInfo ssaoBufferImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(ssaoTexture.getLayout())
         .setImageView(ssaoTexture.getDefaultView())
         .setSampler(normalSampler);
      std::array<vk::WriteDescriptorSet, 2> descriptorWrites =
      {
         vk::WriteDescriptorSet()
            .setDstSet(forwardDescriptorSet.getCurrentSet())
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&normalBufferImageInfo),
         vk::WriteDescriptorSet()
            .setDstSet(forwardDescriptorSet.getCurrentSet())
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&ssaoBufferImageInfo)
      };
      device.updateDescriptorSets(descriptorWrites, {});

      {
         SCOPED_LABEL("Opaque");
         renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);
      }

      {
         SCOPED_LABEL("Masked");
         renderMeshes<BlendMode::Masked>(commandBuffer, sceneRenderInfo);
      }

      {
         SCOPED_LABEL("Translucent");
         renderMeshes<BlendMode::Translucent>(commandBuffer, sceneRenderInfo);
      }

      if (skyboxTexture)
      {
         SCOPED_LABEL("Skybox");

         vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
            .setImageLayout(skyboxTexture->getLayout())
            .setImageView(skyboxTexture->getDefaultView())
            .setSampler(skyboxSampler);
         vk::WriteDescriptorSet descriptorWrite = vk::WriteDescriptorSet()
            .setDstSet(skyboxDescriptorSet.getCurrentSet())
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&imageInfo);
         device.updateDescriptorSets(descriptorWrite, {});

         PipelineDescription<ForwardPass> pipelineDescription;
         pipelineDescription.withBlending = false;
         pipelineDescription.withTextures = true;
         pipelineDescription.skybox = true;

         skyboxShader->bindDescriptorSets(commandBuffer, skyboxPipelineLayout, sceneRenderInfo.view, skyboxDescriptorSet);
         renderScreenMesh(commandBuffer, getPipeline(pipelineDescription));
      }
   });
}

void ForwardPass::renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   ASSERT(lighting);
   forwardShader->bindDescriptorSets(commandBuffer, pipeline.getLayout(), view, forwardDescriptorSet, *lighting, material);

   SceneRenderPass::renderMesh(commandBuffer, pipeline, view, mesh, section, material);
}

vk::PipelineLayout ForwardPass::selectPipelineLayout(BlendMode blendMode) const
{
   return forwardPipelineLayout;
}

PipelineDescription<ForwardPass> ForwardPass::getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const
{
   PipelineDescription<ForwardPass> description;

   description.withTextures = meshSection.hasValidTexCoords;
   description.withBlending = material.getBlendMode() == BlendMode::Translucent;
   description.twoSided = material.isTwoSided();

   return description;
}

Pipeline ForwardPass::createPipeline(const PipelineDescription<ForwardPass>& description, const AttachmentFormats& attachmentFormats)
{
   std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates;
   if (description.withBlending)
   {
      blendAttachmentStates.push_back(vk::PipelineColorBlendAttachmentState()
         .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
         .setBlendEnable(true)
         .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
         .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
         .setColorBlendOp(vk::BlendOp::eAdd)
         .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
         .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
         .setAlphaBlendOp(vk::BlendOp::eAdd));
   }
   else
   {
      blendAttachmentStates.push_back(vk::PipelineColorBlendAttachmentState()
         .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
         .setBlendEnable(false));
   }
   blendAttachmentStates.push_back(blendAttachmentStates[0]);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = description.skybox ? PipelinePassType::Screen : PipelinePassType::Mesh;
   pipelineInfo.enableDepthTest = true;
   pipelineInfo.twoSided = description.twoSided;

   PipelineData pipelineData(attachmentFormats);
   pipelineData.layout = description.skybox ? skyboxPipelineLayout : forwardPipelineLayout;
   pipelineData.shaderStages = description.skybox ? skyboxShader->getStages() : forwardShader->getStages(description.withTextures, description.withBlending);
   pipelineData.colorBlendStates = blendAttachmentStates;

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.skybox ? "Skybox" : (std::string(description.withTextures ? "With" : "Without") + " Textures, " + std::string(description.withBlending ? "With" : "Without") + " Blending" + (description.twoSided ? ", Two Sided" : "")));

   return pipeline;
}
