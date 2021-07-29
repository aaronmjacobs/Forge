#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <vector>

class Framebuffer : public GraphicsResource
{
public:
   Framebuffer(const GraphicsContext& graphicsContext, vk::RenderPass renderPass, const AttachmentInfo& attachmentInfo);
   Framebuffer(Framebuffer&& other);
   ~Framebuffer();

   vk::Framebuffer getCurrentFramebuffer() const;

   vk::Extent2D getExtent() const
   {
      return extent;
   }

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

private:
   std::vector<vk::Framebuffer> framebuffers;

   vk::Extent2D extent;
   bool hasSwapchainAttachment = false;
};
