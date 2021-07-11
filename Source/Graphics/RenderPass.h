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

protected:
   virtual void initializePipelines(vk::SampleCountFlagBits sampleCount) = 0;
   void initializeRenderPass(const RenderPassAttachments& passAttachments);
   void initializeFramebuffers(const RenderPassAttachments& passAttachments);

   void terminateRenderPass();
   void terminateFramebuffers();
   void terminatePipelines();

   virtual std::vector<vk::SubpassDependency> getSubpassDependencies() const = 0;

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
