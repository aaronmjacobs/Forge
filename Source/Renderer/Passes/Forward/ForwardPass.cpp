#include "Renderer/Passes/Forward/ForwardPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/Passes/Forward/ForwardShader.h"
#include "Renderer/Passes/Forward/SkyboxShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

#include <utility>

ForwardPass::ForwardPass(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, ResourceManager& resourceManager, const ForwardLighting* forwardLighting)
   : SceneRenderPass(graphicsContext)
   , lighting(forwardLighting)
   , forwardDescriptorSet(graphicsContext, dynamicDescriptorPool, ForwardShader::getLayoutCreateInfo())
   , skyboxDescriptorSet(graphicsContext, dynamicDescriptorPool, SkyboxShader::getLayoutCreateInfo())
{
   clearDepth = false;
   clearColor = true;

   forwardShader = std::make_unique<ForwardShader>(context, resourceManager);
   skyboxShader = std::make_unique<SkyboxShader>(context, resourceManager);

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

   forwardShader.reset();
   skyboxShader.reset();
}

void ForwardPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle, Texture& normalBuffer, Texture& ssaoBuffer, const Texture* skyboxTexture)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   vk::ImageLayout normalBufferInitialLayout = normalBuffer.getLayout();
   if (normalBufferInitialLayout != vk::ImageLayout::eShaderReadOnlyOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      normalBuffer.transitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }

   vk::ImageLayout ssaoBufferInitialLayout = ssaoBuffer.getLayout();
   if (ssaoBufferInitialLayout != vk::ImageLayout::eShaderReadOnlyOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      ssaoBuffer.transitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }

   std::array<float, 4> clearColorValues = { 1.0f, 1.0f, 1.0f, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0), vk::ClearColorValue(clearColorValues) };

   beginRenderPass(commandBuffer, *framebuffer, clearValues);

   vk::DescriptorImageInfo normalBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(normalBuffer.getLayout())
      .setImageView(normalBuffer.getDefaultView())
      .setSampler(normalSampler);
   vk::DescriptorImageInfo ssaoBufferImageInfo = vk::DescriptorImageInfo()
      .setImageLayout(ssaoBuffer.getLayout())
      .setImageView(ssaoBuffer.getDefaultView())
      .setSampler(normalSampler);
   std::vector<vk::WriteDescriptorSet> descriptorWrites =
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

   endRenderPass(commandBuffer);

   if (normalBufferInitialLayout == vk::ImageLayout::eColorAttachmentOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      normalBuffer.transitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }

   if (ssaoBufferInitialLayout == vk::ImageLayout::eColorAttachmentOptimal)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      ssaoBuffer.transitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }
}

std::vector<vk::SubpassDependency> ForwardPass::getSubpassDependencies() const
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

Pipeline ForwardPass::createPipeline(const PipelineDescription<ForwardPass>& description)
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

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = description.skybox ? PipelinePassType::Screen : PipelinePassType::Mesh;
   pipelineInfo.enableDepthTest = true;
   pipelineInfo.twoSided = description.twoSided;

   PipelineData pipelineData;
   pipelineData.renderPass = getRenderPass();
   pipelineData.layout = description.skybox ? skyboxPipelineLayout : forwardPipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.shaderStages = description.skybox ? skyboxShader->getStages() : forwardShader->getStages(description.withTextures, description.withBlending);
   pipelineData.colorBlendStates = { blendAttachmentStates };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, description.skybox ? "Skybox" : (std::string(description.withTextures ? "With" : "Without") + " Textures, " + std::string(description.withBlending ? "With" : "Without") + " Blending" + (description.twoSided ? ", Two Sided" : "")));

   return pipeline;
}
