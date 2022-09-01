#include "Graphics/RenderPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include <optional>

namespace
{
   bool formatsMatch(std::span<const AttachmentInfo> attachments, std::span<vk::Format> formats)
   {
      if (attachments.size() != formats.size())
      {
         return false;
      }

      for (std::size_t i = 0; i < attachments.size(); ++i)
      {
         if (!attachments[i].matchesFormat(formats[i]))
         {
            return false;
         }
      }

      return true;
   }

   bool extentsMatch(std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment)
   {
      std::optional<vk::Extent2D> extent;

      for (const AttachmentInfo& colorAttachment : colorAttachments)
      {
         if (colorAttachment.texture)
         {
            if (extent.has_value() && colorAttachment.texture->getExtent() != extent)
            {
               return false;
            }
            extent = colorAttachment.texture->getExtent();
         }
      }

      if (depthStencilAttachment && depthStencilAttachment->texture)
      {
         if (extent.has_value() && depthStencilAttachment->texture->getExtent() != extent)
         {
            return false;
         }
      }

      return true;
   }

   vk::Extent2D getAttachmentExtent(std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment)
   {
      ASSERT(extentsMatch(colorAttachments, depthStencilAttachment));

      for (const AttachmentInfo& colorAttachment : colorAttachments)
      {
         if (colorAttachment.texture)
         {
            return colorAttachment.texture->getExtent();
         }
      }

      if (depthStencilAttachment && depthStencilAttachment->texture)
      {
         return depthStencilAttachment->texture->getExtent();
      }

      return vk::Extent2D(0, 0);
   }
}

AttachmentFormats::AttachmentFormats(std::span<const Texture*> colorAttachments, const Texture* depthStencilAttachment)
{
   for (const Texture* colorAttachment : colorAttachments)
   {
      if (colorAttachment)
      {
         colorFormats.push(colorAttachment->getImageProperties().format);
      }
   }

   if (depthStencilAttachment)
   {
      depthStencilFormat = depthStencilAttachment->getImageProperties().format;
   }

   for (const Texture* colorAttachment : colorAttachments)
   {
      if (colorAttachment)
      {
         sampleCount = colorAttachment->getTextureProperties().sampleCount;
         break;
      }
   }
   if (depthStencilAttachment)
   {
      sampleCount = depthStencilAttachment->getTextureProperties().sampleCount;
   }

#if FORGE_DEBUG
   for (const Texture* colorAttachment : colorAttachments)
   {
      if (colorAttachment)
      {
         ASSERT(sampleCount == colorAttachment->getTextureProperties().sampleCount, "Not all attachments have the same sample count");
      }
   }

   if (depthStencilAttachment)
   {
      ASSERT(sampleCount == depthStencilAttachment->getTextureProperties().sampleCount, "Not all attachments have the same sample count");
   }
#endif // FORGE_DEBUG
}

AttachmentFormats::AttachmentFormats(const Texture* colorAttachment, const Texture* depthStencilAttachment)
   : AttachmentFormats(colorAttachment ? std::span<const Texture*>(&colorAttachment, 1) : std::span<const Texture*>{}, depthStencilAttachment)
{
}

vk::RenderingAttachmentInfo AttachmentInfo::getRenderingAttachmentInfo() const
{
   return vk::RenderingAttachmentInfo()
      .setImageView(viewOverride ? viewOverride : (texture ? texture->getDefaultView() : nullptr))
      .setImageLayout(texture ? texture->getLayout() : vk::ImageLayout::eUndefined)
      .setResolveMode(resolveMode)
      .setResolveImageView(resolveViewOverride ? resolveViewOverride : (resolveTexture ? resolveTexture->getDefaultView() : nullptr))
      .setResolveImageLayout(resolveTexture ? resolveTexture->getLayout() : vk::ImageLayout::eUndefined)
      .setLoadOp(loadOp)
      .setStoreOp(storeOp)
      .setClearValue(clearValue);
}

bool AttachmentInfo::matchesFormat(vk::Format format) const
{
   return (texture == nullptr && format == vk::Format::eUndefined) || (texture != nullptr && texture->getImageProperties().format == format);
}

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

void RenderPass::updateAttachmentFormats(std::span<const Texture*> colorAttachments, const Texture* depthStencilAttachment)
{
   updateAttachmentFormats(AttachmentFormats(colorAttachments, depthStencilAttachment));
}

void RenderPass::updateAttachmentFormats(const Texture* colorAttachment, const Texture* depthStencilAttachment)
{
   updateAttachmentFormats(AttachmentFormats(colorAttachment, depthStencilAttachment));
}

void RenderPass::beginRenderPass(vk::CommandBuffer commandBuffer, std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment)
{
   ASSERT((depthStencilAttachment == nullptr && attachmentFormats.depthStencilFormat == vk::Format::eUndefined) || (depthStencilAttachment != nullptr && depthStencilAttachment->matchesFormat(attachmentFormats.depthStencilFormat)));
   ASSERT(formatsMatch(colorAttachments, attachmentFormats.colorFormats));

   StaticVector<vk::RenderingAttachmentInfo, kMaxColorAttachments> colorRenderingAttachmentInfo;
   for (const AttachmentInfo& colorAttachment : colorAttachments)
   {
      colorRenderingAttachmentInfo.push(colorAttachment.getRenderingAttachmentInfo());
   }

   vk::RenderingAttachmentInfo depthStencilRenderingAttachmentInfo;
   if (depthStencilAttachment)
   {
      depthStencilRenderingAttachmentInfo = depthStencilAttachment->getRenderingAttachmentInfo();
   }

   vk::Extent2D extent = getAttachmentExtent(colorAttachments, depthStencilAttachment);
   vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

   vk::RenderingInfo renderingInfo = vk::RenderingInfo()
      .setRenderArea(renderArea)
      .setLayerCount(1)
      .setPDepthAttachment(depthStencilAttachment ? &depthStencilRenderingAttachmentInfo : nullptr)
      .setPStencilAttachment(depthStencilAttachment ? &depthStencilRenderingAttachmentInfo : nullptr)
      .setColorAttachments(colorRenderingAttachmentInfo);

   commandBuffer.beginRendering(renderingInfo, GraphicsContext::GetDynamicLoader());

   setViewport(commandBuffer, renderArea);
}

void RenderPass::beginRenderPass(vk::CommandBuffer commandBuffer, const AttachmentInfo* colorAttachment, const AttachmentInfo* depthStencilAttachment)
{
   if (colorAttachment)
   {
      beginRenderPass(commandBuffer, std::span<const AttachmentInfo, 1>(colorAttachment, 1), depthStencilAttachment);
   }
   else
   {
      beginRenderPass(commandBuffer, std::span<const AttachmentInfo, 0>{}, depthStencilAttachment);
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
