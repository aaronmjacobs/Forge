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

   depthShader = std::make_unique<DepthShader>(context, resourceManager);
   depthMaskedShader = std::make_unique<DepthMaskedShader>(context, resourceManager);

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = depthShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      opaquePipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(opaquePipelineLayout, "Opaque Pipeline Layout");
   }

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthMaskedShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = depthMaskedShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      maskedPipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(maskedPipelineLayout, "Masked Pipeline Layout");
   }
}

DepthPass::~DepthPass()
{
   context.delayedDestroy(std::move(opaquePipelineLayout));
   context.delayedDestroy(std::move(maskedPipelineLayout));

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
      PipelineDescription<DepthPass> description;
      description.cubemap = true;
      // Workaround for bug in MoltenVK - need to set the pipeline before setting the depth bias
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, getPipeline(description));
#endif // defined(__APPLE__)

      const ViewInfo& viewInfo = sceneRenderInfo.view.getInfo();
      commandBuffer.setDepthBias(viewInfo.depthBiasConstantFactor, viewInfo.depthBiasClamp, viewInfo.depthBiasSlopeFactor);
   }

   {
      SCOPED_LABEL("Opaque");

      depthShader->bindDescriptorSets(commandBuffer, opaquePipelineLayout, sceneRenderInfo.view);
      renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);
   }

   {
      SCOPED_LABEL("Masked");

      renderMeshes<BlendMode::Masked>(commandBuffer, sceneRenderInfo);
   }

   endRenderPass(commandBuffer);
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

void DepthPass::renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   if (pipeline.getLayout() == maskedPipelineLayout)
   {
      depthMaskedShader->bindDescriptorSets(commandBuffer, pipeline.getLayout(), view, material);
   }

   SceneRenderPass::renderMesh(commandBuffer, pipeline, view, mesh, section, material);
}

vk::PipelineLayout DepthPass::selectPipelineLayout(BlendMode blendMode) const
{
   return blendMode == BlendMode::Masked ? maskedPipelineLayout : opaquePipelineLayout;
}

PipelineDescription<DepthPass> DepthPass::getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const
{
   PipelineDescription<DepthPass> description;

   description.masked = material.getBlendMode() == BlendMode::Masked;
   description.twoSided = material.isTwoSided();
   description.cubemap = view.getInfo().cubeFace.has_value();

   return description;
}

Pipeline DepthPass::createPipeline(const PipelineDescription<DepthPass>& description)
{
   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Mesh;
   pipelineInfo.enableDepthTest = true;
   pipelineInfo.writeDepth = true;
   pipelineInfo.enableDepthBias = isShadowPass;
   pipelineInfo.positionOnly = !description.masked;
   pipelineInfo.twoSided = description.twoSided;
   pipelineInfo.swapFrontFace = description.cubemap; // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing

   PipelineData pipelineData;
   pipelineData.renderPass = getRenderPass();
   pipelineData.layout = description.masked ? maskedPipelineLayout : opaquePipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.shaderStages = description.masked ? depthMaskedShader->getStages() : depthShader->getStages();
   pipelineData.colorBlendStates = {};

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, std::string(description.masked ? "" : "Not ") + "Masked, " + std::string(description.twoSided ? "" : "Not ") + "Two Sided, " + std::string(description.cubemap ? "" : "Not ") + "Cubemap");

   return pipeline;
}
