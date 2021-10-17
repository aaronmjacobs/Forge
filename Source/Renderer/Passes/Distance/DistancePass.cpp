#include "Renderer/Passes/Distance/DistancePass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Pipeline.h"

#include "Renderer/Passes/Distance/DistanceShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/View.h"

#include <limits>

DistancePass::DistancePass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : SceneRenderPass(graphicsContext)
{
   clearDepth = true;
   clearColor = true;
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

   static const float kMaxDistance = std::numeric_limits<float>::max();
   std::array<float, 4> clearColorValues = { kMaxDistance, kMaxDistance, kMaxDistance, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0), vk::ClearColorValue(clearColorValues) };
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
      .setColorWriteMask(vk::ColorComponentFlagBits::eR)
      .setBlendEnable(false);

   PipelineData pipelineData(context, pipelineLayout, getRenderPass(), PipelinePassType::Mesh, distanceShader->getStages(), { attachmentState }, sampleCount, true);
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
      .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
      .setDstAccessMask(vk::AccessFlagBits::eShaderRead));

   return subpassDependencies;
}

vk::Pipeline DistancePass::selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const
{
   return view.getInfo().cubeFace.has_value() ? pipelines[1] : pipelines[0];
}
