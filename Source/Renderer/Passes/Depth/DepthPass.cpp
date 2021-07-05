#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

DepthPass::DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& depthTexture)
   : GraphicsResource(graphicsContext)
{
   depthShader = std::make_unique<DepthShader>(context, resourceManager);

   initializeSwapchainDependentResources(depthTexture);
}

DepthPass::~DepthPass()
{
   terminateSwapchainDependentResources();

   depthShader.reset();
}

void DepthPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
{
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(renderPass)
      .setFramebuffer(framebuffer)
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

   depthShader->bindDescriptorSets(commandBuffer, sceneRenderInfo.view, pipelineLayout);

   for (const MeshRenderInfo& meshRenderInfo : sceneRenderInfo.meshes)
   {
      if (!meshRenderInfo.visibleOpaqueSections.empty())
      {
         MeshUniformData meshUniformData;
         meshUniformData.localToWorld = meshRenderInfo.localToWorld;
         commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

         for (uint32_t section : meshRenderInfo.visibleOpaqueSections)
         {
            meshRenderInfo.mesh.bindBuffers(commandBuffer, section);
            meshRenderInfo.mesh.draw(commandBuffer, section);
         }
      }
   }

   commandBuffer.endRenderPass();
}

void DepthPass::onSwapchainRecreated(const Texture& depthTexture)
{
   terminateSwapchainDependentResources();
   initializeSwapchainDependentResources(depthTexture);
}

void DepthPass::initializeSwapchainDependentResources(const Texture& depthTexture)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();

   {
      vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
         .setFormat(depthTexture.getImageProperties().format)
         .setSamples(depthTexture.getTextureProperties().sampleCount)
         .setLoadOp(vk::AttachmentLoadOp::eClear)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
         .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
         .setInitialLayout(vk::ImageLayout::eUndefined)
         .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      vk::AttachmentReference depthAttachmentReference = vk::AttachmentReference()
         .setAttachment(0)
         .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      vk::SubpassDescription subpassDescription = vk::SubpassDescription()
         .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
         .setPDepthStencilAttachment(&depthAttachmentReference);

      vk::SubpassDependency subpassDependency = vk::SubpassDependency()
         .setSrcSubpass(VK_SUBPASS_EXTERNAL)
         .setDstSubpass(0)
         .setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
         .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
         .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
         .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

      vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
         .setAttachments(depthAttachment)
         .setSubpasses(subpassDescription)
         .setDependencies(subpassDependency);

      renderPass = device.createRenderPass(renderPassCreateInfo);
   }

   {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = depthShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

      PipelineData pipelineData(context, pipelineLayout, renderPass, depthShader->getStages(), {}, depthTexture.getTextureProperties().sampleCount);
      pipeline = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
   }

   {
      std::array<vk::ImageView, 1> attachments = { depthTexture.getDefaultView() };

      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachments(attachments)
         .setWidth(swapchainExtent.width)
         .setHeight(swapchainExtent.height)
         .setLayers(1);

      framebuffer = device.createFramebuffer(framebufferCreateInfo);
   }
}

void DepthPass::terminateSwapchainDependentResources()
{
   if (framebuffer)
   {
      device.destroyFramebuffer(framebuffer);
      framebuffer = nullptr;
   }

   if (pipeline)
   {
      device.destroyPipeline(pipeline);
      pipeline = nullptr;
   }
   if (pipelineLayout)
   {
      device.destroyPipelineLayout(pipelineLayout);
      pipelineLayout = nullptr;
   }

   if (renderPass)
   {
      device.destroyRenderPass(renderPass);
      renderPass = nullptr;
   }
}

