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

void NormalPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, Texture& depthTexture, Texture& normalTexture)
{
   SCOPED_LABEL(getName());

   depthTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);
   normalTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   vk::RenderingAttachmentInfo depthStencilAttachmentInfo = vk::RenderingAttachmentInfo()
      .setImageView(depthTexture.getDefaultView())
      .setImageLayout(depthTexture.getLayout())
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

   vk::RenderingAttachmentInfo colorAttachmentInfo = vk::RenderingAttachmentInfo()
      .setImageView(normalTexture.getDefaultView())
      .setImageLayout(normalTexture.getLayout())
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }));

   ASSERT(depthTexture.getExtent() == normalTexture.getExtent());
   beginRenderPass(commandBuffer, depthTexture.getExtent(), &depthStencilAttachmentInfo, &colorAttachmentInfo);

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
   pipelineData.layout = pipelineLayout;
   pipelineData.sampleCount = getSampleCount();
   pipelineData.depthStencilFormat = getDepthStencilFormat();
   pipelineData.colorFormats = getColorFormats();
   pipelineData.shaderStages = normalShader->getStages(description.withTextures, description.masked);
   pipelineData.colorBlendStates = { attachmentState };

   Pipeline pipeline(context, pipelineInfo, pipelineData);
   NAME_CHILD(pipeline, std::string(description.withTextures ? "With" : "Without") + " Textures, " + std::string(description.masked ? "Masked" : "") + (description.twoSided ? ", Two Sided" : ""));

   return pipeline;
}
