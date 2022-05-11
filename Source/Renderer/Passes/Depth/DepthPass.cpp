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

void DepthPass::renderMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   if (pipelineLayout == maskedPipelineLayout)
   {
      depthMaskedShader->bindDescriptorSets(commandBuffer, pipelineLayout, view, material);
   }

   SceneRenderPass::renderMesh(commandBuffer, pipelineLayout, view, mesh, section, material);
}

vk::PipelineLayout DepthPass::selectPipelineLayout(BlendMode blendMode) const
{
   return blendMode == BlendMode::Masked ? maskedPipelineLayout : opaquePipelineLayout;
}

PipelineDescription<DepthPass> DepthPass::getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const
{
   PipelineDescription<DepthPass> description;

   description.masked = material.getBlendMode() == BlendMode::Masked;
   description.cubemap = view.getInfo().cubeFace.has_value();

   return description;
}

vk::Pipeline DepthPass::createPipeline(const PipelineDescription<DepthPass>& description)
{
   PipelineInfo pipelineInfo;
   pipelineInfo.renderPass = getRenderPass();
   pipelineInfo.layout = description.masked ? maskedPipelineLayout : opaquePipelineLayout;
   pipelineInfo.sampleCount = getSampleCount();
   pipelineInfo.passType = PipelinePassType::Mesh;
   pipelineInfo.writeDepth = true;
   pipelineInfo.positionOnly = !description.masked;

   PipelineData pipelineData(pipelineInfo, description.masked ? depthMaskedShader->getStages() : depthShader->getStages(), {});

   if (isShadowPass)
   {
      pipelineData.enableDepthBias();
   }

   if (description.cubemap)
   {
      pipelineData.setFrontFace(vk::FrontFace::eClockwise); // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing
   }

   vk::Pipeline pipeline = device.createGraphicsPipeline(context.getPipelineCache(), pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipeline, "Pipeline (" + std::string(description.masked ? "" : "Not ") + "Masked, " + std::string(description.cubemap ? "" : "Not ") + "Cubemap)");

   return pipeline;
}
