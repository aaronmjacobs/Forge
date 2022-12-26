#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthMaskedShader.h"
#include "Renderer/Passes/Depth/DepthShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

DepthPass::DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, bool shadowPass)
   : SceneRenderPass(graphicsContext)
   , isShadowPass(shadowPass)
{
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

void DepthPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, vk::ImageView depthTextureView)
{
   SCOPED_LABEL(getName());

   depthTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo depthStencilAttachmentInfo = AttachmentInfo(depthTexture)
      .setViewOverride(depthTextureView)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

   executePass(commandBuffer, nullptr, &depthStencilAttachmentInfo, [this, &sceneRenderInfo](vk::CommandBuffer commandBuffer)
   {
      if (isShadowPass)
      {
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
   });
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

Pipeline DepthPass::createPipeline(const PipelineDescription<DepthPass>& description, const AttachmentFormats& attachmentFormats)
{
   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Mesh;
   pipelineInfo.enableDepthTest = true;
   pipelineInfo.writeDepth = true;
   pipelineInfo.enableDepthBias = isShadowPass;
   pipelineInfo.positionOnly = !description.masked;
   pipelineInfo.twoSided = description.twoSided;
   pipelineInfo.swapFrontFace = description.cubemap; // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing

   PipelineData pipelineData(attachmentFormats);
   pipelineData.layout = description.masked ? maskedPipelineLayout : opaquePipelineLayout;
   pipelineData.shaderStages = description.masked ? depthMaskedShader->getStages() : depthShader->getStages();
   pipelineData.colorBlendStates = {};

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, std::string(description.masked ? "" : "Not ") + "Masked, " + std::string(description.twoSided ? "" : "Not ") + "Two Sided, " + std::string(description.cubemap ? "" : "Not ") + "Cubemap");

   return pipeline;
}
