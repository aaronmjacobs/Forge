#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <vector>

class Framebuffer : public GraphicsResource
{
public:
   Framebuffer(const GraphicsContext& graphicsContext, vk::RenderPass renderPass, const AttachmentInfo& attachInfo);
   Framebuffer(Framebuffer&& other);
   ~Framebuffer();

   vk::Framebuffer getCurrentFramebuffer() const;

   const AttachmentInfo& getAttachmentInfo() const
   {
      return attachmentInfo;
   }

   vk::Extent2D getExtent() const
   {
      return extent;
   }

private:
   std::vector<vk::Framebuffer> framebuffers;

   const AttachmentInfo attachmentInfo;
   vk::Extent2D extent;
   bool hasSwapchainAttachment = false;
};
