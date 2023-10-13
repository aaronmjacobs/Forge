#include "Graphics/RenderPass.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include <optional>

namespace
{
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

   StaticVector<const Texture*, kMaxColorAttachments> getAttachmentTextures(std::span<const AttachmentInfo> attachments)
   {
      StaticVector<const Texture*, kMaxColorAttachments> textures;

      for (const AttachmentInfo& attachment : attachments)
      {
         textures.push(attachment.texture);
      }

      return textures;
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

#if FORGE_WITH_DEBUG_UTILS
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
#endif // FORGE_WITH_DEBUG_UTILS
}

AttachmentFormats::AttachmentFormats(const Texture* colorAttachment, const Texture* depthStencilAttachment)
   : AttachmentFormats(colorAttachment ? std::span<const Texture*>(&colorAttachment, 1) : std::span<const Texture*>{}, depthStencilAttachment)
{
}

AttachmentFormats::AttachmentFormats(std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment)
   : AttachmentFormats(getAttachmentTextures(colorAttachments), depthStencilAttachment ? depthStencilAttachment->texture : nullptr)
{
}

AttachmentFormats::AttachmentFormats(const AttachmentInfo* colorAttachment, const AttachmentInfo* depthStencilAttachment)
   : AttachmentFormats(colorAttachment ? std::span<const AttachmentInfo>(colorAttachment, 1) : std::span<const AttachmentInfo>{}, depthStencilAttachment)
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

void RenderPass::beginRenderPass(vk::CommandBuffer commandBuffer, std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment)
{
   ASSERT(!attachmentFormats.has_value(), "Beginning a render pass, but another render pass is already in progress");
   attachmentFormats = AttachmentFormats(colorAttachments, depthStencilAttachment);

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

   onRenderPassBegin();
}

void RenderPass::endRenderPass(vk::CommandBuffer commandBuffer)
{
   ASSERT(attachmentFormats.has_value());
   attachmentFormats.reset();

   commandBuffer.endRendering(GraphicsContext::GetDynamicLoader());

   onRenderPassEnd();
}
