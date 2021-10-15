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

   bool operator==(const FramebufferHandle& other) const
   {
      return id == other.id;
   }

   std::size_t hash() const
   {
      return Hash::of(id);
   }

private:
   static uint64_t counter;

   uint64_t id = 0;
};

namespace std
{
   template<>
   struct hash<FramebufferHandle>
   {
      size_t operator()(const FramebufferHandle& value) const
      {
         return value.hash();
      }
   };
}

class RenderPass : public GraphicsResource
{
public:
   RenderPass(const GraphicsContext& graphicsContext);
   ~RenderPass();

   void updateAttachmentSetup(const BasicAttachmentInfo& setup);

   FramebufferHandle createFramebuffer(const AttachmentInfo& attachmentInfo);
   void destroyFramebuffer(FramebufferHandle& handle);

protected:
   void initializeRenderPass();
   virtual void initializePipelines(vk::SampleCountFlagBits sampleCount) = 0;

   void terminateRenderPass();
   void terminatePipelines();

   virtual std::vector<vk::SubpassDependency> getSubpassDependencies() const = 0;

   void beginRenderPass(vk::CommandBuffer commandBuffer, const Framebuffer& framebuffer, std::span<vk::ClearValue> clearValues);
   void endRenderPass(vk::CommandBuffer commandBuffer);
   void setViewport(vk::CommandBuffer commandBuffer, const vk::Rect2D& rect);

   vk::RenderPass getRenderPass() const { return renderPass; }
   const Framebuffer* getFramebuffer(FramebufferHandle handle) const;

   vk::PipelineLayout pipelineLayout;
   std::vector<vk::Pipeline> pipelines;

   bool clearDepth = false;
   bool clearColor = false;

private:
   vk::RenderPass renderPass;
   std::unordered_map<FramebufferHandle, Framebuffer> framebufferMap;

   BasicAttachmentInfo attachmentSetup;
};
