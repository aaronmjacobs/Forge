#include "Graphics/Framebuffer.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"

#include <utility>

namespace
{
   bool attachmentsReferenceSwapchain(const AttachmentInfo& attachmentInfo)
   {
      ASSERT(!attachmentInfo.depthInfo.has_value() || !attachmentInfo.depthInfo->isSwapchainTexture);

      for (const TextureInfo& colorInfo : attachmentInfo.colorInfo)
      {
         if (colorInfo.isSwapchainTexture)
         {
            return true;
         }
      }

      for (const TextureInfo& resolveInfo : attachmentInfo.resolveInfo)
      {
         if (resolveInfo.isSwapchainTexture)
         {
            return true;
         }
      }

      return false;
   }
}

Framebuffer::Framebuffer(const GraphicsContext& graphicsContext, vk::RenderPass renderPass, const AttachmentInfo& attachmentInfo)
   : GraphicsResource(graphicsContext)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();
   const std::vector<vk::ImageView>& swapchainImageViews = context.getSwapchain().getImageViews();

   hasSwapchainAttachment = attachmentsReferenceSwapchain(attachmentInfo);
   uint32_t numFramebuffers = hasSwapchainAttachment ? context.getSwapchain().getImageCount() : 1;
   framebuffers.resize(numFramebuffers);

   for (uint32_t i = 0; i < numFramebuffers; ++i)
   {
      std::optional<vk::Extent2D> attachmentsExtent;

      std::vector<vk::ImageView> attachments;
      attachments.reserve((attachmentInfo.depthInfo ? 1 : 0) + attachmentInfo.colorInfo.size() + attachmentInfo.resolveInfo.size());

      if (attachmentInfo.depthInfo)
      {
         ASSERT(!attachmentInfo.depthInfo->isSwapchainTexture);
         ASSERT(attachmentInfo.depthInfo->view);
         attachments.push_back(attachmentInfo.depthInfo->view);

         attachmentsExtent = vk::Extent2D(attachmentInfo.depthInfo->extent);
      }

      for (const TextureInfo& colorInfo : attachmentInfo.colorInfo)
      {
         if (colorInfo.isSwapchainTexture)
         {
            ASSERT(hasSwapchainAttachment);
            attachments.push_back(swapchainImageViews[i]);

            ASSERT(!attachmentsExtent || attachmentsExtent.value() == swapchainExtent);
            attachmentsExtent = swapchainExtent;
         }
         else
         {
            ASSERT(colorInfo.view);
            attachments.push_back(colorInfo.view);

            ASSERT(!attachmentsExtent || attachmentsExtent.value() == colorInfo.extent);
            attachmentsExtent = colorInfo.extent;
         }
      }

      for (const TextureInfo& resolveInfo : attachmentInfo.resolveInfo)
      {
         if (resolveInfo.isSwapchainTexture)
         {
            ASSERT(hasSwapchainAttachment);
            attachments.push_back(swapchainImageViews[i]);

            ASSERT(!attachmentsExtent || attachmentsExtent.value() == swapchainExtent);
            attachmentsExtent = swapchainExtent;
         }
         else
         {
            ASSERT(resolveInfo.view);
            attachments.push_back(resolveInfo.view);

            ASSERT(!attachmentsExtent || attachmentsExtent.value() == resolveInfo.extent);
            attachmentsExtent = resolveInfo.extent;
         }
      }

      ASSERT(attachmentsExtent.has_value());
      extent = attachmentsExtent.value();

      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachments(attachments)
         .setWidth(extent.width)
         .setHeight(extent.height)
         .setLayers(1);

      ASSERT(!framebuffers[i]);
      framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
   }
}

Framebuffer::Framebuffer(Framebuffer&& other)
   : GraphicsResource(other.context)
   , framebuffers(std::move(other.framebuffers))
   , extent(other.extent)
   , hasSwapchainAttachment(other.hasSwapchainAttachment)
{
}

Framebuffer::~Framebuffer()
{
   for (vk::Framebuffer& framebuffer : framebuffers)
   {
      context.delayedDestroy(std::move(framebuffer));
   }
}

vk::Framebuffer Framebuffer::getCurrentFramebuffer() const
{
   uint32_t index = hasSwapchainAttachment ? context.getSwapchainIndex() : 0;
   ASSERT(framebuffers.size() > index);

   return framebuffers[index];
}

#if FORGE_DEBUG
void Framebuffer::setName(std::string_view newName)
{
   GraphicsResource::setName(newName);

   for (std::size_t i = 0; i < framebuffers.size(); ++i)
   {
      NAME_OBJECT(framebuffers[i], name + " Index " + std::to_string(i));
   }
}
#endif // FORGE_DEBUG
