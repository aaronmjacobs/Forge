#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

DepthPass::DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : RenderPass(graphicsContext)
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

void DepthPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
{
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(getRenderPass())
      .setFramebuffer(getCurrentFramebuffer())
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[0]);

   depthShader->bindDescriptorSets(commandBuffer, sceneRenderInfo.view, pipelineLayout);

   for (const MeshRenderInfo& meshRenderInfo : sceneRenderInfo.meshes)
   {
      if (!meshRenderInfo.visibleOpaqueSections.empty())
      {
         ASSERT(meshRenderInfo.mesh);

         MeshUniformData meshUniformData;
         meshUniformData.localToWorld = meshRenderInfo.localToWorld;
         commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

         for (uint32_t section : meshRenderInfo.visibleOpaqueSections)
         {
            meshRenderInfo.mesh->bindBuffers(commandBuffer, section);
            meshRenderInfo.mesh->draw(commandBuffer, section);
         }
      }
   }

   commandBuffer.endRenderPass();
}

void DepthPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthShader->getSetLayouts();
   std::vector<vk::PushConstantRange> pushConstantRanges = depthShader->getPushConstantRanges();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts)
      .setPushConstantRanges(pushConstantRanges);
   pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

   PipelineData pipelineData(context, pipelineLayout, getRenderPass(), depthShader->getStages(), {}, sampleCount);
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
