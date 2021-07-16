#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/Texture.h"

#include <optional>
#include <vector>

struct RenderPassAttachments
{
   std::optional<TextureInfo> depthInfo;
   std::vector<TextureInfo> colorInfo;
   std::vector<TextureInfo> resolveInfo;
};

class RenderPass : public GraphicsResource
{
public:
   RenderPass(const GraphicsContext& graphicsContext);
   virtual ~RenderPass();

   void updateAttachments(const RenderPassAttachments& passAttachments);

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

protected:
   void initializeRenderPass(const RenderPassAttachments& passAttachments);
   virtual void initializePipelines(vk::SampleCountFlagBits sampleCount) = 0;
   void initializeFramebuffers(const RenderPassAttachments& passAttachments);

   void terminateRenderPass();
   void terminatePipelines();
   void terminateFramebuffers();

   virtual std::vector<vk::SubpassDependency> getSubpassDependencies() const = 0;
   virtual void postUpdateAttachments() {}

   vk::RenderPass getRenderPass() const { return renderPass; }
   vk::Framebuffer getCurrentFramebuffer() const;

   vk::PipelineLayout pipelineLayout;
   std::vector<vk::Pipeline> pipelines;

   bool clearDepth = false;
   bool clearColor = false;

private:
   vk::RenderPass renderPass;
   std::vector<vk::Framebuffer> framebuffers;

   RenderPassAttachments lastPassAttachments;
   bool hasSwapchainAttachment = false;
};
