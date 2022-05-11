#pragma once

#include "Core/Hash.h"

#include "Graphics/Framebuffer.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <span>
#include <unordered_map>
#include <vector>

class FramebufferHandle
{
public:
   static FramebufferHandle create();

   void invalidate()
   {
      id = 0;
   }

   bool isValid() const
   {
      return id != 0;
   }

   uint64_t getId() const
   {
      return id;
   }

   explicit operator bool() const
   {
      return isValid();
   }

   bool operator==(const FramebufferHandle& other) const = default;

   std::size_t hash() const
   {
      return Hash::of(id);
   }

private:
   static uint64_t counter;

   uint64_t id = 0;
};

USE_MEMBER_HASH_FUNCTION(FramebufferHandle);

class RenderPass : public GraphicsResource
{
public:
   RenderPass(const GraphicsContext& graphicsContext);
   ~RenderPass();

   void updateAttachmentSetup(const BasicAttachmentInfo& setup);

   FramebufferHandle createFramebuffer(const AttachmentInfo& attachmentInfo);
   void destroyFramebuffer(FramebufferHandle& handle);

   vk::RenderPass getRenderPass() const
   {
      return renderPass;
   }

   vk::SampleCountFlagBits getSampleCount() const
   {
      return sampleCount;
   }

   void setIsFinalRenderPass(bool isFinalPass)
   {
      isFinalRenderPass = isFinalPass;
   }

protected:
   void initializeRenderPass();
   void terminateRenderPass();

   virtual void postRenderPassInitialized() {}

   virtual std::vector<vk::SubpassDependency> getSubpassDependencies() const = 0;

   void beginRenderPass(vk::CommandBuffer commandBuffer, const Framebuffer& framebuffer, std::span<vk::ClearValue> clearValues);
   void endRenderPass(vk::CommandBuffer commandBuffer);
   void setViewport(vk::CommandBuffer commandBuffer, const vk::Rect2D& rect);

   const Framebuffer* getFramebuffer(FramebufferHandle handle) const;

   bool clearDepth = false;
   bool clearColor = false;

private:
   vk::RenderPass renderPass;
   std::unordered_map<FramebufferHandle, Framebuffer> framebufferMap;

   BasicAttachmentInfo attachmentSetup;
   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
   bool isFinalRenderPass = false;
};
