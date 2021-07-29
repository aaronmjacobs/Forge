#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

DepthPass::DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
{
   clearDepth = true;
   clearColor = false;
   pipelines.resize(1);

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

   depthShader->bindDescriptorSets(commandBuffer, sceneRenderInfo.view, pipelineLayout);

   renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);

   endRenderPass(commandBuffer);
}

#if FORGE_DEBUG
void DepthPass::setName(std::string_view newName)
{
   SceneRenderPass::setName(newName);

   NAME_OBJECT(pipelines[0], name + " Pipeline");
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
   pipelines[0] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
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
