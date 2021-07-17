#include "Renderer/Passes/Forward/ForwardPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Forward/ForwardShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

namespace
{
   uint32_t getForwardPipelineIndex(bool withTextures, bool withBlending)
   {
      return (withTextures ? 0b01 : 0b00) | (withBlending ? 0b10 : 0b00);
   }
}

ForwardPass::ForwardPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
   , lighting(graphicsContext, descriptorPool)
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

void ForwardPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
{
   SCOPED_LABEL("Forward pass");

   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0), vk::ClearColorValue(clearColorValues) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(getRenderPass())
      .setFramebuffer(getCurrentFramebuffer())
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   INLINE_LABEL("Update lighting");
   lighting.update(sceneRenderInfo);

   {
      SCOPED_LABEL("Opaque");
      renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);
   }

   {
      SCOPED_LABEL("Translucent");
      renderMeshes<BlendMode::Translucent>(commandBuffer, sceneRenderInfo);
   }

   commandBuffer.endRenderPass();
}

#if FORGE_DEBUG
void ForwardPass::setName(std::string_view newName)
{
   SceneRenderPass::setName(newName);

   NAME_OBJECT(pipelines[getForwardPipelineIndex(false, false)], name + " Pipeline (Without Textures, Without Blending)");
   NAME_OBJECT(pipelines[getForwardPipelineIndex(true, false)], name + " Pipeline (With Textures, Without Blending)");
   NAME_OBJECT(pipelines[getForwardPipelineIndex(true, true)], name + " Pipeline (With Textures, With Blending)");
   NAME_OBJECT(pipelines[getForwardPipelineIndex(false, true)], name + " Pipeline (Without Textures, With Blending)");
}
#endif // FORGE_DEBUG

void ForwardPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = forwardShader->getSetLayouts();
   std::vector<vk::PushConstantRange> pushConstantRanges = forwardShader->getPushConstantRanges();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts)
      .setPushConstantRanges(pushConstantRanges);
   pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

   std::vector<vk::PipelineShaderStageCreateInfo> shaderStagesWithoutTextures = forwardShader->getStages(false);
   std::vector<vk::PipelineShaderStageCreateInfo> shaderStagesWithTextures = forwardShader->getStages(true);

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

   PipelineData pipelineData(context, pipelineLayout, getRenderPass(), PipelinePassType::Mesh, shaderStagesWithoutTextures, colorBlendDisabledAttachments, sampleCount);
   pipelines[getForwardPipelineIndex(false, false)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

   pipelineData.setShaderStages(shaderStagesWithTextures);
   pipelines[getForwardPipelineIndex(true, false)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

   pipelineData.setColorBlendAttachmentStates(colorBlendEnabledAttachments);
   pipelines[getForwardPipelineIndex(true, true)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

   pipelineData.setShaderStages(shaderStagesWithoutTextures);
   pipelines[getForwardPipelineIndex(false, true)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
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

void ForwardPass::postUpdateAttachments()
{
   NAME_OBJECT(*this, "Forward Pass");
}

void ForwardPass::renderMesh(vk::CommandBuffer commandBuffer, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   forwardShader->bindDescriptorSets(commandBuffer, pipelineLayout, view, lighting, material);

   SceneRenderPass::renderMesh(commandBuffer, view, mesh, section, material);
}

vk::Pipeline ForwardPass::selectPipeline(const MeshSection& meshSection, const Material& material) const
{
   return pipelines[getForwardPipelineIndex(meshSection.hasValidTexCoords, material.getBlendMode() == BlendMode::Translucent)];
}
