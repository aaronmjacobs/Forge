#include "Renderer/Passes/Distance/DistancePass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"

#include "Renderer/Passes/Distance/DistanceShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

DistancePass::DistancePass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
{
   clearDepth = true; // TODO Not sure what format to use for the texture yet (D32F? R32F?)
   clearColor = false;
   pipelines.resize(2);

   distanceShader = std::make_unique<DistanceShader>(context, resourceManager);
}

DistancePass::~DistancePass()
{
   distanceShader.reset();
}

void DistancePass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle)
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

   const ViewInfo& viewInfo = sceneRenderInfo.view.getInfo();
   commandBuffer.setDepthBias(viewInfo.depthBiasConstantFactor, viewInfo.depthBiasClamp, viewInfo.depthBiasSlopeFactor);

   distanceShader->bindDescriptorSets(commandBuffer, pipelineLayout, sceneRenderInfo.view);

   renderMeshes<BlendMode::Opaque>(commandBuffer, sceneRenderInfo);

   endRenderPass(commandBuffer);
}

void DistancePass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = distanceShader->getSetLayouts();
   std::vector<vk::PushConstantRange> pushConstantRanges = distanceShader->getPushConstantRanges();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts)
      .setPushConstantRanges(pushConstantRanges);
   pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

   vk::PipelineColorBlendAttachmentState attachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   PipelineData pipelineData(context, pipelineLayout, getRenderPass(), PipelinePassType::Mesh, distanceShader->getStages(), {}, sampleCount); // TODO Need attachment state?
   pipelineData.enableDepthBias(); // TODO does this even work?

   pipelines[0] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[0], "Pipeline");

   pipelineData.setFrontFace(vk::FrontFace::eClockwise); // Projection matrix Y values will be inverted when rendering to a cubemap, which swaps which faces are "front" facing
   pipelines[1] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   NAME_CHILD(pipelines[1], "Pipeline (Cubemap)");
}

std::vector<vk::SubpassDependency> DistancePass::getSubpassDependencies() const
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

vk::Pipeline DistancePass::selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const
{
   return view.getInfo().cubeFace.has_value() ? pipelines[1] : pipelines[0];
}
