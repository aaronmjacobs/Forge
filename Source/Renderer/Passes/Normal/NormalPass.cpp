#include "Renderer/Passes/Normal/NormalPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Normal/NormalShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

#include <utility>

NormalPass::NormalPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
{
   clearDepth = true;
   clearColor = true;

   normalShader = std::make_unique<NormalShader>(context, resourceManager);

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = normalShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = normalShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
      NAME_CHILD(pipelineLayout, "Pipeline Layout");
   }
}

NormalPass::~NormalPass()
{
   context.delayedDestroy(std::move(pipelineLayout));

   normalShader.reset();
}

void NormalPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
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

   endRenderPass(commandBuffer);
}

std::vector<vk::SubpassDependency> NormalPass::getSubpassDependencies() const
{
   std::vector<vk::SubpassDependency> subpassDependencies;

   subpassDependencies.push_back(vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite));

   return subpassDependencies;
}

void NormalPass::renderMesh(vk::CommandBuffer commandBuffer, const Pipeline& pipeline, const View& view, const Mesh& mesh, uint32_t section, const Material& material)
{
   normalShader->bindDescriptorSets(commandBuffer, pipeline.getLayout(), view, material);

   SceneRenderPass::renderMesh(commandBuffer, pipeline, view, mesh, section, material);
}

vk::PipelineLayout NormalPass::selectPipelineLayout(BlendMode blendMode) const
{
   return pipelineLayout;
}

PipelineDescription<NormalPass> NormalPass::getPipelineDescription(const View& view, const MeshSection& meshSection, const Material& material) const
{
   PipelineDescription<NormalPass> description;

   description.withTextures = meshSection.hasValidTexCoords;
   description.masked = material.getBlendMode() == BlendMode::Masked;
   description.twoSided = material.isTwoSided();

   return description;
}

Pipeline NormalPass::createPipeline(const PipelineDescription<NormalPass>& description)
{
   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineInfo pipelineInfo;
   pipelineInfo.passType = PipelinePassType::Mesh;
   pipelineInfo.enableDepthTest = true;
   pipelineInfo.writeDepth = true;
   pipelineInfo.twoSided = description.twoSided;

   PipelineData pipelineData;
   pipelineData.renderPass = getRenderPass();
   pipelineData.layout = pipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.shaderStages = normalShader->getStages(description.withTextures, description.masked);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, std::string(description.withTextures ? "With" : "Without") + " Textures, " + std::string(description.masked ? "Masked" : "") + (description.twoSided ? ", Two Sided" : ""));

   return pipeline;
}
