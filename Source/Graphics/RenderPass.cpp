#include "Graphics/RenderPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"

#include <utility>

namespace
{
   std::optional<vk::SampleCountFlagBits> getSampleCount(const BasicAttachmentInfo& info)
   {
      std::optional<vk::SampleCountFlagBits> sampleCount;

      if (info.depthInfo.has_value())
      {
         sampleCount = info.depthInfo->sampleCount;
      }

      for (const BasicTextureInfo& colorInfo : info.colorInfo)
      {
         ASSERT(!sampleCount.has_value() || sampleCount.value() == colorInfo.sampleCount);
         sampleCount = colorInfo.sampleCount;
      }

      return sampleCount;
   }
}

FramebufferHandle FramebufferHandle::create()
{
   FramebufferHandle handle;
   handle.id = ++counter;

   return handle;
}

uint64_t FramebufferHandle::counter = 0;

RenderPass::RenderPass(const GraphicsContext& graphicsContext)
   : GraphicsResource(graphicsContext)
{
}

RenderPass::~RenderPass()
{
   framebufferMap.clear();
   terminatePipelines();
   terminateRenderPass();
}

void RenderPass::updateAttachmentSetup(const BasicAttachmentInfo& setup)
{
   framebufferMap.clear();
   terminatePipelines();
   terminateRenderPass();

   attachmentSetup = setup;

   initializeRenderPass();
   initializePipelines(getSampleCount(attachmentSetup).value_or(vk::SampleCountFlagBits::e1));

   for (std::size_t i = 0; i < pipelineLayouts.size(); ++i)
   {
      NAME_CHILD(pipelineLayouts[i], "Pipeline Layout " + DebugUtils::toString(i));
   }
}

FramebufferHandle RenderPass::createFramebuffer(const AttachmentInfo& attachmentInfo)
{
   ASSERT(attachmentSetup == attachmentInfo.asBasic());

   FramebufferHandle handle = FramebufferHandle::create();
   auto result = framebufferMap.emplace(handle, Framebuffer(context, renderPass, attachmentInfo));
   NAME_CHILD(result.first->second, "Framebuffer " + DebugUtils::toString(result.first->first.getId()));

   return handle;
}

void RenderPass::destroyFramebuffer(FramebufferHandle& handle)
{
   std::size_t numErased = framebufferMap.erase(handle);
   if (numErased > 0)
   {
      handle.invalidate();
   }
}

void RenderPass::initializeRenderPass()
{
   ASSERT(!renderPass);

   std::vector<vk::AttachmentDescription> attachments;
   vk::SubpassDescription subpassDescription = vk::SubpassDescription()
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

   std::optional<vk::AttachmentReference> depthAttachmentReference;
   if (attachmentSetup.depthInfo)
   {
      vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
         .setFormat(attachmentSetup.depthInfo->format)
         .setSamples(attachmentSetup.depthInfo->sampleCount)
         .setLoadOp(clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
         .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
         .setInitialLayout(clearDepth ? vk::ImageLayout::eUndefined : vk::ImageLayout::eDepthStencilAttachmentOptimal)
         .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      depthAttachmentReference = vk::AttachmentReference()
         .setAttachment(static_cast<uint32_t>(attachments.size()))
         .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      attachments.push_back(depthAttachment);
      subpassDescription.setPDepthStencilAttachment(&depthAttachmentReference.value());
   }

   std::vector<vk::AttachmentReference> colorAttachmentReferences;
   for (const BasicTextureInfo& colorInfo : attachmentSetup.colorInfo)
   {
      vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
         .setFormat(colorInfo.format)
         .setSamples(colorInfo.sampleCount)
         .setLoadOp(clearColor ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
         .setInitialLayout(clearColor ? vk::ImageLayout::eUndefined : vk::ImageLayout::eColorAttachmentOptimal)
         .setFinalLayout(colorInfo.isSwapchainTexture ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eColorAttachmentOptimal);

      vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
         .setAttachment(static_cast<uint32_t>(attachments.size()))
         .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

      attachments.push_back(colorAttachment);
      colorAttachmentReferences.push_back(colorAttachmentReference);
   }
   if (!colorAttachmentReferences.empty())
   {
      subpassDescription.setColorAttachments(colorAttachmentReferences);
   }

   std::vector<vk::AttachmentReference> resolveAttachmentReferences;
   for (const BasicTextureInfo& resolveInfo : attachmentSetup.resolveInfo)
   {
      vk::AttachmentDescription resolveAttachment = vk::AttachmentDescription()
         .setFormat(resolveInfo.format)
         .setSamples(resolveInfo.sampleCount)
         .setLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
         .setInitialLayout(vk::ImageLayout::eUndefined)
         .setFinalLayout(resolveInfo.isSwapchainTexture ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eColorAttachmentOptimal);

      vk::AttachmentReference resolveAttachmentReference = vk::AttachmentReference()
         .setAttachment(static_cast<uint32_t>(attachments.size()))
         .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

      attachments.push_back(resolveAttachment);
      resolveAttachmentReferences.push_back(resolveAttachmentReference);
   }
   if (!resolveAttachmentReferences.empty())
   {
      subpassDescription.setResolveAttachments(resolveAttachmentReferences);
   }

   std::vector<vk::SubpassDependency> subpassDependencies = getSubpassDependencies();

   vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
      .setAttachments(attachments)
      .setSubpasses(subpassDescription)
      .setDependencies(subpassDependencies);

   renderPass = device.createRenderPass(renderPassCreateInfo);
   NAME_CHILD(renderPass, "Render Pass");
}

void RenderPass::terminateRenderPass()
{
   if (renderPass)
   {
      context.delayedDestroy(std::move(renderPass));
   }
}

void RenderPass::terminatePipelines()
{
   for (vk::Pipeline& pipeline : pipelines)
   {
      if (pipeline)
      {
         context.delayedDestroy(std::move(pipeline));
      }
   }

   for (vk::PipelineLayout& pipelineLayout : pipelineLayouts)
   {
      context.delayedDestroy(std::move(pipelineLayout));
   }
   pipelineLayouts.clear();
}

void RenderPass::beginRenderPass(vk::CommandBuffer commandBuffer, const Framebuffer& framebuffer, std::span<vk::ClearValue> clearValues)
{
   vk::Rect2D renderArea(vk::Offset2D(0, 0), framebuffer.getExtent());

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(getRenderPass())
      .setFramebuffer(framebuffer.getCurrentFramebuffer())
      .setRenderArea(renderArea)
      .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
      .setPClearValues(clearValues.data());
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   setViewport(commandBuffer, renderArea);
}

void RenderPass::endRenderPass(vk::CommandBuffer commandBuffer)
{
   commandBuffer.endRenderPass();
}

void RenderPass::setViewport(vk::CommandBuffer commandBuffer, const vk::Rect2D& rect)
{
   SCOPED_LABEL("Set viewport");

   vk::Viewport viewport = vk::Viewport()
      .setX(static_cast<float>(rect.offset.x))
      .setY(static_cast<float>(rect.offset.y))
      .setWidth(static_cast<float>(rect.extent.width))
      .setHeight(static_cast<float>(rect.extent.height))
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);
   commandBuffer.setViewport(0, viewport);

   commandBuffer.setScissor(0, rect);
}

const Framebuffer* RenderPass::getFramebuffer(FramebufferHandle handle) const
{
   auto location = framebufferMap.find(handle);
   return location == framebufferMap.end() ? nullptr : &location->second;
}
