#pragma once

#include "Core/Assert.h"
#include "Core/Containers/StaticVector.h"
#include "Core/Hash.h"

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <optional>
#include <span>

constexpr std::size_t kMaxColorAttachments = 4;

struct AttachmentInfo
{
   Texture* texture = nullptr;
   vk::ImageView viewOverride = nullptr;

   Texture* resolveTexture = nullptr;
   vk::ImageView resolveViewOverride = nullptr;
   vk::ResolveModeFlagBits resolveMode = vk::ResolveModeFlagBits::eNone;

   vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eLoad;
   vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
   vk::ClearValue clearValue = {};

   AttachmentInfo() = default;

   explicit AttachmentInfo(Texture& tex)
      : texture(&tex)
   {
   }

   AttachmentInfo& setTexture(Texture& texture_)
   {
      texture = &texture_;
      return *this;
   }

   AttachmentInfo& setViewOverride(vk::ImageView viewOverride_)
   {
      viewOverride = viewOverride_;
      return *this;
   }

   AttachmentInfo& setResolveTexture(Texture& resolveTexture_)
   {
      resolveTexture = &resolveTexture_;
      return *this;
   }

   AttachmentInfo& setResolveViewOverride(vk::ImageView resolveViewOverride_)
   {
      resolveViewOverride = resolveViewOverride_;
      return *this;
   }

   AttachmentInfo& setResolveMode(vk::ResolveModeFlagBits resolveMode_)
   {
      resolveMode = resolveMode_;
      return *this;
   }

   AttachmentInfo& setLoadOp(vk::AttachmentLoadOp loadOp_)
   {
      loadOp = loadOp_;
      return *this;
   }

   AttachmentInfo& setStoreOp(vk::AttachmentStoreOp storeOp_)
   {
      storeOp = storeOp_;
      return *this;
   }

   AttachmentInfo& setClearValue(vk::ClearValue clearValue_)
   {
      clearValue = clearValue_;
      return *this;
   }

   vk::RenderingAttachmentInfo getRenderingAttachmentInfo() const;
   bool matchesFormat(vk::Format format) const;
};

struct AttachmentFormats
{
   StaticVector<vk::Format, kMaxColorAttachments> colorFormats;
   vk::Format depthStencilFormat = vk::Format::eUndefined;

   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

   AttachmentFormats() = default;
   AttachmentFormats(std::span<const Texture*> colorAttachments, const Texture* depthStencilAttachment = nullptr);
   AttachmentFormats(const Texture* colorAttachment, const Texture* depthStencilAttachment = nullptr);
   AttachmentFormats(std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment = nullptr);
   AttachmentFormats(const AttachmentInfo* colorAttachment, const AttachmentInfo* depthStencilAttachment = nullptr);

   std::size_t hash() const
   {
      std::size_t hashValue = Hash::of(colorFormats);
      Hash::combine(hashValue, depthStencilFormat);
      Hash::combine(hashValue, sampleCount);

      return hashValue;
   }

   bool operator==(const AttachmentFormats& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(AttachmentFormats);

class RenderPass : public GraphicsResource
{
public:
   RenderPass(const GraphicsContext& graphicsContext);

protected:
   virtual void onRenderPassBegin() {}
   virtual void onRenderPassEnd() {}

   template<typename Func>
   void executePass(vk::CommandBuffer commandBuffer, std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment, Func&& func)
   {
      beginRenderPass(commandBuffer, colorAttachments, depthStencilAttachment);

      func(commandBuffer);

      endRenderPass(commandBuffer);
   }

   template<typename Func>
   void executePass(vk::CommandBuffer commandBuffer, const AttachmentInfo* colorAttachment, const AttachmentInfo* depthStencilAttachment, Func&& func)
   {
      if (colorAttachment)
      {
         executePass(commandBuffer, std::span<const AttachmentInfo, 1>(colorAttachment, 1), depthStencilAttachment, std::move(func));
      }
      else
      {
         executePass(commandBuffer, std::span<const AttachmentInfo, 0>{}, depthStencilAttachment, std::move(func));
      }
   }

   void setViewport(vk::CommandBuffer commandBuffer, const vk::Rect2D& rect);

   const AttachmentFormats& getAttachmentFormats() const
   {
      ASSERT(attachmentFormats.has_value(), "Calling getAttachmentFormats() outside of a render pass");
      return attachmentFormats.value();
   }

private:
   void beginRenderPass(vk::CommandBuffer commandBuffer, std::span<const AttachmentInfo> colorAttachments, const AttachmentInfo* depthStencilAttachment);
   void endRenderPass(vk::CommandBuffer commandBuffer);

   std::optional<AttachmentFormats> attachmentFormats;
};
