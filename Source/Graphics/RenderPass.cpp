#include "Graphics/RenderPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"

#include <utility>

RenderPass::RenderPass(const GraphicsContext& graphicsContext)
   : GraphicsResource(graphicsContext)
{
}

void RenderPass::updateAttachmentFormats(const AttachmentFormats& formats)
{
   if (formats != attachmentFormats)
   {
      attachmentFormats = formats;

      postUpdateAttachmentFormats();
   }
}

void RenderPass::updateAttachmentFormats(const Texture* depthStencilAttachment, std::span<const Texture> colorAttachments)
{
   updateAttachmentFormats(AttachmentFormats(depthStencilAttachment, colorAttachments));
}

void RenderPass::updateAttachmentFormats(const Texture* depthStencilAttachment, const Texture* colorAttachment)
{
   updateAttachmentFormats(AttachmentFormats(depthStencilAttachment, colorAttachment));
}

void RenderPass::beginRenderPass(vk::CommandBuffer commandBuffer, vk::Extent2D extent, const vk::RenderingAttachmentInfo* depthStencilAttachment, std::span<const vk::RenderingAttachmentInfo> colorAttachments)
{
   vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

   vk::RenderingInfo renderingInfo = vk::RenderingInfo()
      .setRenderArea(renderArea)
      .setLayerCount(1)
      .setPDepthAttachment(depthStencilAttachment)
      .setPStencilAttachment(depthStencilAttachment)
      .setColorAttachments(colorAttachments);

   commandBuffer.beginRendering(renderingInfo, GraphicsContext::GetDynamicLoader());

   setViewport(commandBuffer, renderArea);
}

void RenderPass::beginRenderPass(vk::CommandBuffer commandBuffer, vk::Extent2D extent, const vk::RenderingAttachmentInfo* depthStencilAttachment, const vk::RenderingAttachmentInfo* colorAttachment)
{
   if (colorAttachment)
   {
      beginRenderPass(commandBuffer, extent, depthStencilAttachment, std::span<const vk::RenderingAttachmentInfo, 1>(colorAttachment, 1));
   }
   else
   {
      beginRenderPass(commandBuffer, extent, depthStencilAttachment, std::span<const vk::RenderingAttachmentInfo, 0>{});
   }
}

void RenderPass::endRenderPass(vk::CommandBuffer commandBuffer)
{
   commandBuffer.endRendering(GraphicsContext::GetDynamicLoader());
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
