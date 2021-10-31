#include "Renderer/Passes/Forward/ForwardPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/Passes/Forward/ForwardShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

ForwardPass::ForwardPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const ForwardLighting* forwardLighting)
   : SceneRenderPass(graphicsContext)
   , lighting(forwardLighting)
{
   clearDepth = false;
   clearColor = true;
   pipelines.resize(4);

   forwardShader = std::make_unique<ForwardShader>(context, resourceManager);
}

ForwardPass::~ForwardPass()
{
   forwardShader.reset();
}

void ForwardPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle)
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

   endRenderPass(commandBuffer);
}

void ForwardPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   pipelineLayouts.resize(1);

   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = forwardShader->getSetLayouts();
   std::vector<vk::PushConstantRange> pushConstantRanges = forwardShader->getPushConstantRanges();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts)
      .setPushConstantRanges(pushConstantRanges);
   pipelineLayouts[0] = device.createPipelineLayout(pipelineLayoutCreateInfo);

   std::array<std::vector<vk::PipelineShaderStageCreateInfo>, 4> shaderStages =
   {
      forwardShader->getStages(false, false),
      forwardShader->getStages(false, true),
      forwardShader->getStages(true, false),
      forwardShader->getStages(true, true)
   };

   vk::PipelineColorBlendAttachmentState colorBlendDisabledAttachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);
   vk::PipelineColorBlendAttachmentState colorBlendEnabledAttachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(true)
      .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd);
   std::vector<vk::PipelineColorBlendAttachmentState> colorBlendDisabledAttachments = { colorBlendDisabledAttachmentState };
   std::vector<vk::PipelineColorBlendAttachmentState> colorBlendEnabledAttachments = { colorBlendEnabledAttachmentState };

   PipelineInfo pipelineInfo;
   pipelineInfo.renderPass = getRenderPass();
   pipelineInfo.layout = pipelineLayouts[0];
   pipelineInfo.sampleCount = sampleCount;
   pipelineInfo.passType = PipelinePassType::Mesh;
   pipelineInfo.writeDepth = false;
   pipelineInfo.positionOnly = false;

   PipelineData pipelineData(context, pipelineInfo, shaderStages[ForwardShader::getPermutationIndex(false, false)], colorBlendDisabledAttachments);
   pipelines[ForwardShader::getPermutationIndex(false, false)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[ForwardShader::getPermutationIndex(false, false)], "Pipeline (Without Textures, Without Blending)");

   pipelineData.setShaderStages(shaderStages[ForwardShader::getPermutationIndex(true, false)]);
   pipelines[ForwardShader::getPermutationIndex(true, false)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[ForwardShader::getPermutationIndex(true, false)], "Pipeline (With Textures, Without Blending)");

   pipelineData.setShaderStages(shaderStages[ForwardShader::getPermutationIndex(true, true)]);
   pipelineData.setColorBlendAttachmentStates(colorBlendEnabledAttachments);
   pipelines[ForwardShader::getPermutationIndex(true, true)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[ForwardShader::getPermutationIndex(true, true)], "Pipeline (With Textures, With Blending)");

   pipelineData.setShaderStages(shaderStages[ForwardShader::getPermutationIndex(false, true)]);
   pipelines[ForwardShader::getPermutationIndex(false, true)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[ForwardShader::getPermutationIndex(false, true)], "Pipeline (Without Textures, With Blending)");
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

vk::Pipeline ForwardPass::selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const
{
   return pipelines[ForwardShader::getPermutationIndex(meshSection.hasValidTexCoords, material.getBlendMode() == BlendMode::Translucent)];
}
