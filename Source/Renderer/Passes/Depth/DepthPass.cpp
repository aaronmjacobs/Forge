#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

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
   pipelines.resize(2);

   depthShader = std::make_unique<DepthShader>(context, resourceManager);
}

DepthPass::~DepthPass()
{
   depthShader.reset();
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
      const ViewInfo& viewInfo = sceneRenderInfo.view.getInfo();
      commandBuffer.setDepthBias(viewInfo.depthBiasConstantFactor, viewInfo.depthBiasClamp, viewInfo.depthBiasSlopeFactor);
   }

   depthShader->bindDescriptorSets(commandBuffer, sceneRenderInfo.view, pipelineLayout);

   renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);

   endRenderPass(commandBuffer);
}

#if FORGE_DEBUG
void DepthPass::setName(std::string_view newName)
{
   SceneRenderPass::setName(newName);

   NAME_OBJECT(pipelines[0], name + " Pipeline");
   NAME_OBJECT(pipelines[1], name + " Pipeline (Cubemap)");
}
#endif // FORGE_DEBUG

void DepthPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthShader->getSetLayouts();
   std::vector<vk::PushConstantRange> pushConstantRanges = depthShader->getPushConstantRanges();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts)
      .setPushConstantRanges(pushConstantRanges);
   pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

   PipelineData pipelineData(context, pipelineLayout, getRenderPass(), PipelinePassType::Mesh, depthShader->getStages(), {}, sampleCount);
   if (isShadowPass)
   {
      pipelineData.enableDepthBias();
   }

   pipelines[0] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

   pipelineData.setFrontFace(vk::FrontFace::eClockwise); // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing
   pipelines[1] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
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

vk::Pipeline DepthPass::selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const
{
   return view.getInfo().cubeFace.has_value() ? pipelines[1] : pipelines[0];
}
