#pragma once

#include "Core/Hash.h"

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <span>

class RenderPass : public GraphicsResource
{
public:
   RenderPass(const GraphicsContext& graphicsContext);

   void updateAttachmentFormats(const AttachmentFormats& formats);
   void updateAttachmentFormats(const Texture* depthStencilAttachment, std::span<const Texture> colorAttachments);
   void updateAttachmentFormats(const Texture* depthStencilAttachment, const Texture* colorAttachment);

   vk::SampleCountFlagBits getSampleCount() const
   {
      return attachmentFormats.sampleCount;
   }

protected:
   virtual void postUpdateAttachmentFormats() {}

   void beginRenderPass(vk::CommandBuffer commandBuffer, vk::Extent2D extent, const vk::RenderingAttachmentInfo* depthStencilAttachment, std::span<const vk::RenderingAttachmentInfo> colorAttachments);
   void beginRenderPass(vk::CommandBuffer commandBuffer, vk::Extent2D extent, const vk::RenderingAttachmentInfo* depthStencilAttachment, const vk::RenderingAttachmentInfo* colorAttachment);
   void endRenderPass(vk::CommandBuffer commandBuffer);
   void setViewport(vk::CommandBuffer commandBuffer, const vk::Rect2D& rect);

   vk::Format getDepthStencilFormat() const
   {
      return attachmentFormats.depthStencilFormat;
   }

   std::span<const vk::Format> getColorFormats() const
   {
      return attachmentFormats.colorFormats;
   }

private:
   AttachmentFormats attachmentFormats;
};
