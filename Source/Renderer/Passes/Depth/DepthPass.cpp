#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthMaskedShader.h"
#include "Renderer/Passes/Depth/DepthShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

DepthPass::DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, bool shadowPass)
   : SceneRenderPass(graphicsContext)
   , isShadowPass(shadowPass)
{
   clearDepth = true;
   clearColor = false;
   pipelines.resize(4);

   depthShader = std::make_unique<DepthShader>(context, resourceManager);
   depthMaskedShader = std::make_unique<DepthMaskedShader>(context, resourceManager);
}

DepthPass::~DepthPass()
{
   depthShader.reset();
   depthMaskedShader.reset();
}

void DepthPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0) };
   beginRenderPass(commandBuffer, *framebuffer, clearValues);

   if (isShadowPass)
   {
#if defined(__APPLE__)
      // Workaround for bug in MoltenVK - need to set the pipeline before setting the depth bias
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[1]);
#endif // defined(__APPLE__)

      const ViewInfo& viewInfo = sceneRenderInfo.view.getInfo();
      commandBuffer.setDepthBias(viewInfo.depthBiasConstantFactor, viewInfo.depthBiasClamp, viewInfo.depthBiasSlopeFactor);
   }

   {
      SCOPED_LABEL("Opaque");

      depthShader->bindDescriptorSets(commandBuffer, pipelineLayouts[0], sceneRenderInfo.view);
      renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);
   }

   {
      SCOPED_LABEL("Masked");

      renderMeshes<BlendMode::Masked>(commandBuffer, sceneRenderInfo);
   }

   endRenderPass(commandBuffer);
}

void DepthPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   pipelineLayouts.resize(2);

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = depthShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      pipelineLayouts[0] = device.createPipelineLayout(pipelineLayoutCreateInfo);
   }

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthMaskedShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = depthMaskedShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      pipelineLayouts[1] = device.createPipelineLayout(pipelineLayoutCreateInfo);
   }

   {
      PipelineInfo pipelineInfo;
      pipelineInfo.renderPass = getRenderPass();
      pipelineInfo.layout = pipelineLayouts[0];
      pipelineInfo.sampleCount = sampleCount;
      pipelineInfo.passType = PipelinePassType::Mesh;
      pipelineInfo.writeDepth = true;
      pipelineInfo.positionOnly = true;

      PipelineData pipelineData(pipelineInfo, depthShader->getStages(), {});
      if (isShadowPass)
      {
         pipelineData.enableDepthBias();
      }

      pipelines[0] = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
      NAME_CHILD(pipelines[0], "Pipeline");

      pipelineData.setFrontFace(vk::FrontFace::eClockwise); // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing
      pipelines[1] = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
      NAME_CHILD(pipelines[1], "Pipeline (Cubemap)");
   }

   {
      PipelineInfo pipelineInfo;
      pipelineInfo.renderPass = getRenderPass();
      pipelineInfo.layout = pipelineLayouts[1];
      pipelineInfo.sampleCount = sampleCount;
      pipelineInfo.passType = PipelinePassType::Mesh;
      pipelineInfo.writeDepth = true;
      pipelineInfo.positionOnly = false;

      PipelineData pipelineData(pipelineInfo, depthMaskedShader->getStages(), {});
      if (isShadowPass)
      {
         pipelineData.enableDepthBias();
      }

      pipelines[2] = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
      NAME_CHILD(pipelines[2], "Masked Pipeline");

      pipelineData.setFrontFace(vk::FrontFace::eClockwise); // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing
      pipelines[3] = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
      NAME_CHILD(pipelines[3], "Masked Pipeline (Cubemap)");
   }
}

std::vector<vk::SubpassDependency> DepthPass::getSubpassDependencies() const
{
   std::vector<vk::SubpassDependency> subpassDependencies;

   subpassDependencies.push_back(vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
      .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
      .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite));

   return subpassDependencies;
}

void DepthPass::renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   if (pipelineLayout == pipelineLayouts[1])
   {
      depthMaskedShader->bindDescriptorSets(commandBuffer, pipelineLayout, view, material);
   }

   SceneRenderPass::renderMesh(commandBuffer, pipelineLayout, view, mesh, section, material);
}

vk::PipelineLayout DepthPass::selectPipelineLayout(BlendMode blendMode) const
{
   uint32_t index = blendMode == BlendMode::Masked ? 1 : 0;
   return pipelineLayouts[index];
}

vk::Pipeline DepthPass::selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const
{
   bool isMasked = material.getBlendMode() == BlendMode::Masked;
   uint32_t index = (0b10 * isMasked) | (0b01 * view.getInfo().cubeFace.has_value());

   return pipelines[index];
}
