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
}

ForwardPass::~ForwardPass()
{
   context.delayedDestroy(std::move(skyboxSampler));

   context.delayedDestroy(std::move(forwardPipelineLayout));
   context.delayedDestroy(std::move(skyboxPipelineLayout));

   forwardShader.reset();
   skyboxShader.reset();
}

void ForwardPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle, const Texture* skyboxTexture)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   std::array<float, 4> clearColorValues = { 1.0f, 1.0f, 1.0f, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0), vk::ClearColorValue(clearColorValues) };

   beginRenderPass(commandBuffer, *framebuffer, clearValues);

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

void ForwardPass::renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   ASSERT(lighting);
   forwardShader->bindDescriptorSets(commandBuffer, pipelineLayout, view, *lighting, material);

   SceneRenderPass::renderMesh(commandBuffer, pipelineLayout, view, mesh, section, material);
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

   return description;
}

vk::Pipeline ForwardPass::createPipeline(const PipelineDescription<ForwardPass>& description)
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

   vk::Pipeline pipeline;
   if (description.skybox)
   {
      PipelineInfo pipelineInfo;
      pipelineInfo.renderPass = getRenderPass();
      pipelineInfo.layout = skyboxPipelineLayout;
      pipelineInfo.sampleCount = getSampleCount();
      pipelineInfo.passType = PipelinePassType::Screen;
      pipelineInfo.enableDepthTest = true;
      pipelineInfo.writeDepth = false;
      pipelineInfo.positionOnly = false;

      PipelineData pipelineData(pipelineInfo, skyboxShader->getStages(), { blendAttachmentStates });
      pipeline = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
      NAME_CHILD(pipeline, "Skybox Pipeline");
   }
   else
   {
      PipelineInfo pipelineInfo;
      pipelineInfo.renderPass = getRenderPass();
      pipelineInfo.layout = forwardPipelineLayout;
      pipelineInfo.sampleCount = getSampleCount();
      pipelineInfo.passType = PipelinePassType::Mesh;
      pipelineInfo.writeDepth = false;
      pipelineInfo.positionOnly = false;

      PipelineData pipelineData(pipelineInfo, forwardShader->getStages(description.withTextures, description.withBlending), blendAttachmentStates);
      pipeline = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
      NAME_CHILD(pipeline, "Pipeline (" + std::string(description.withTextures ? "With" : "Without") + " Textures, " + std::string(description.withBlending ? "With" : "Without") + " Blending)");
   }

   return pipeline;
}
