#include "Graphics/RenderPass.h"

#include "Graphics/Swapchain.h"

namespace
{
   bool attachmentCountsMatch(const RenderPassAttachments& first, const RenderPassAttachments& second)
   {
      return first.depthInfo.has_value() == second.depthInfo.has_value()
         && first.colorInfo.size() == second.colorInfo.size()
         && first.resolveInfo.size() == second.resolveInfo.size();
   }

   template<typename T>
   bool attachmentPropertiesMatch(const RenderPassAttachments& first, const RenderPassAttachments& second, const T& comparator)
   {
      ASSERT(first.depthInfo.has_value() == second.depthInfo.has_value());
      if (first.depthInfo.has_value())
      {
         if (!comparator(first.depthInfo.value(), second.depthInfo.value()))
         {
            return false;
         }
      }

      ASSERT(first.colorInfo.size() == second.colorInfo.size());
      for (uint32_t i = 0; i < first.colorInfo.size(); ++i)
      {
         if (!comparator(first.colorInfo[i], second.colorInfo[i]))
         {
            return false;
         }
      }

      ASSERT(first.resolveInfo.size() == second.resolveInfo.size());
      for (uint32_t i = 0; i < first.resolveInfo.size(); ++i)
      {
         if (!comparator(first.resolveInfo[i], second.resolveInfo[i]))
         {
            return false;
         }
      }

      return true;
   }

   bool swapchainAttachmentsMatch(const RenderPassAttachments& first, const RenderPassAttachments& second)
   {
      static const auto infoMatches = [](const TextureInfo& firstInfo, const TextureInfo& secondInfo)
      {
         return firstInfo.isSwapchainTexture == secondInfo.isSwapchainTexture;
      };

      return attachmentPropertiesMatch(first, second, infoMatches);
   }

   bool formatsAndSampleCountsMatch(const RenderPassAttachments& first, const RenderPassAttachments& second)
   {
      static const auto infoMatches = [](const TextureInfo& firstInfo, const TextureInfo& secondInfo)
      {
         return firstInfo.format == secondInfo.format && firstInfo.sampleCount == secondInfo.sampleCount;
      };

      return attachmentPropertiesMatch(first, second, infoMatches);
   }

   bool viewsAndExtentsMatch(const RenderPassAttachments& first, const RenderPassAttachments& second)
   {
      static const auto infoMatches = [](const TextureInfo& firstInfo, const TextureInfo& secondInfo)
      {
         return firstInfo.view == secondInfo.view && firstInfo.extent == secondInfo.extent;
      };

      return attachmentPropertiesMatch(first, second, infoMatches);
   }

   void determineUpdateRequirements(const RenderPassAttachments& lastPassAttachments, const RenderPassAttachments& newPassAttachments, bool& updateRenderPassAndPipelines, bool& updateFrambuffers)
   {
      if (attachmentCountsMatch(lastPassAttachments, newPassAttachments) && swapchainAttachmentsMatch(lastPassAttachments, newPassAttachments))
      {
         updateRenderPassAndPipelines = !formatsAndSampleCountsMatch(lastPassAttachments, newPassAttachments);
         updateFrambuffers = !viewsAndExtentsMatch(lastPassAttachments, newPassAttachments);
      }
      else
      {
         updateRenderPassAndPipelines = true;
         updateFrambuffers = true;
      }
   }

   std::optional<vk::SampleCountFlagBits> getSampleCountForAttachments(const RenderPassAttachments& passAttachments)
   {
      std::optional<vk::SampleCountFlagBits> sampleCount;

      if (passAttachments.depthInfo.has_value())
      {
         sampleCount = passAttachments.depthInfo->sampleCount;
      }

      for (const TextureInfo& colorInfo : passAttachments.colorInfo)
      {
         ASSERT(!sampleCount.has_value() || sampleCount.value() == colorInfo.sampleCount);
         sampleCount = colorInfo.sampleCount;
      }

      return sampleCount;
   }

   bool attachmentsReferenceSwapchain(const RenderPassAttachments& passAttachments)
   {
      for (const TextureInfo& colorInfo : passAttachments.colorInfo)
      {
         if (colorInfo.isSwapchainTexture)
         {
            return true;
         }
      }

      for (const TextureInfo& resolveInfo : passAttachments.resolveInfo)
      {
         if (resolveInfo.isSwapchainTexture)
         {
            return true;
         }
      }

      return false;
   }
}

RenderPass::RenderPass(const GraphicsContext& graphicsContext)
   : GraphicsResource(graphicsContext)
{
}

RenderPass::~RenderPass()
{
   terminateFramebuffers();
   terminatePipelines();
   terminateRenderPass();
}

void RenderPass::updateAttachments(const RenderPassAttachments& passAttachments)
{
   bool updateRenderPassAndPipelines = false;
   bool updateFrambuffers = false;
   determineUpdateRequirements(lastPassAttachments, passAttachments, updateRenderPassAndPipelines, updateFrambuffers);

   if (updateFrambuffers)
   {
      terminateFramebuffers();
   }
   if (updateRenderPassAndPipelines)
   {
      terminatePipelines();
      terminateRenderPass();
   }

   if (updateRenderPassAndPipelines)
   {
      vk::SampleCountFlagBits newSampleCount = getSampleCountForAttachments(passAttachments).value_or(vk::SampleCountFlagBits::e1);

      initializeRenderPass(passAttachments);
      initializePipelines(newSampleCount);
   }
   if (updateFrambuffers)
   {
      initializeFramebuffers(passAttachments);
   }

   lastPassAttachments = passAttachments;
}

void RenderPass::initializeRenderPass(const RenderPassAttachments& passAttachments)
{
   std::vector<vk::AttachmentDescription> attachments;
   vk::SubpassDescription subpassDescription = vk::SubpassDescription()
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

   std::optional<vk::AttachmentReference> depthAttachmentReference;
   if (passAttachments.depthInfo)
   {
      vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
         .setFormat(passAttachments.depthInfo->format)
         .setSamples(passAttachments.depthInfo->sampleCount)
         .setLoadOp(clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
         .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
         .setInitialLayout(clearDepth ? vk::ImageLayout::eUndefined : vk::ImageLayout::eDepthStencilAttachmentOptimal)
         .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      depthAttachmentReference = vk::AttachmentReference()
         .setAttachment(static_cast<uint32_t>(attachments.size()))
         .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      attachments.push_back(depthAttachment);
      subpassDescription.setPDepthStencilAttachment(&depthAttachmentReference.value());
   }

   std::vector<vk::AttachmentReference> colorAttachmentReferences;
   for (const TextureInfo& colorInfo : passAttachments.colorInfo)
   {
      vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
         .setFormat(colorInfo.format)
         .setSamples(colorInfo.sampleCount)
         .setLoadOp(clearColor ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
         .setInitialLayout(clearColor ? vk::ImageLayout::eUndefined : vk::ImageLayout::eColorAttachmentOptimal)
         .setFinalLayout(colorInfo.isSwapchainTexture ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eColorAttachmentOptimal);

      vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
         .setAttachment(static_cast<uint32_t>(attachments.size()))
         .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

      attachments.push_back(colorAttachment);
      colorAttachmentReferences.push_back(colorAttachmentReference);
   }
   if (!colorAttachmentReferences.empty())
   {
      subpassDescription.setColorAttachments(colorAttachmentReferences);
   }

   std::vector<vk::AttachmentReference> resolveAttachmentReferences;
   for (const TextureInfo& resolveInfo : passAttachments.resolveInfo)
   {
      vk::AttachmentDescription resolveAttachment = vk::AttachmentDescription()
         .setFormat(resolveInfo.format)
         .setSamples(resolveInfo.sampleCount)
         .setLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
         .setInitialLayout(vk::ImageLayout::eUndefined)
         .setFinalLayout(resolveInfo.isSwapchainTexture ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eColorAttachmentOptimal);

      vk::AttachmentReference resolveAttachmentReference = vk::AttachmentReference()
         .setAttachment(static_cast<uint32_t>(attachments.size()))
         .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

      attachments.push_back(resolveAttachment);
      resolveAttachmentReferences.push_back(resolveAttachmentReference);
   }
   if (!resolveAttachmentReferences.empty())
   {
      subpassDescription.setResolveAttachments(resolveAttachmentReferences);
   }

   std::vector<vk::SubpassDependency> subpassDependencies = getSubpassDependencies();

   vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
      .setAttachments(attachments)
      .setSubpasses(subpassDescription)
      .setDependencies(subpassDependencies);

   renderPass = device.createRenderPass(renderPassCreateInfo);
}

void RenderPass::initializeFramebuffers(const RenderPassAttachments& passAttachments)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();

   hasSwapchainAttachment = attachmentsReferenceSwapchain(passAttachments);
   uint32_t numFramebuffers = hasSwapchainAttachment ? context.getSwapchain().getImageCount() : 1;
   framebuffers.resize(numFramebuffers);

   const std::vector<vk::ImageView>& swapchainImageViews = context.getSwapchain().getImageViews();
   for (uint32_t i = 0; i < numFramebuffers; ++i)
   {
      std::optional<vk::Extent2D> framebufferExtent;

      std::vector<vk::ImageView> attachments;
      attachments.reserve((passAttachments.depthInfo ? 1 : 0) + passAttachments.colorInfo.size() + passAttachments.resolveInfo.size());

      if (passAttachments.depthInfo)
      {
         ASSERT(!passAttachments.depthInfo->isSwapchainTexture);
         ASSERT(passAttachments.depthInfo->view);
         attachments.push_back(passAttachments.depthInfo->view);

         framebufferExtent = vk::Extent2D(passAttachments.depthInfo->extent);
      }

      for (const TextureInfo& colorInfo : passAttachments.colorInfo)
      {
         if (colorInfo.isSwapchainTexture)
         {
            ASSERT(hasSwapchainAttachment);
            attachments.push_back(swapchainImageViews[i]);

            ASSERT(!framebufferExtent || framebufferExtent.value() == swapchainExtent);
            framebufferExtent = swapchainExtent;
         }
         else
         {
            ASSERT(colorInfo.view);
            attachments.push_back(colorInfo.view);

            ASSERT(!framebufferExtent || framebufferExtent.value() == colorInfo.extent);
            framebufferExtent = colorInfo.extent;
         }
      }

      for (const TextureInfo& resolveInfo : passAttachments.resolveInfo)
      {
         if (resolveInfo.isSwapchainTexture)
         {
            ASSERT(hasSwapchainAttachment);
            attachments.push_back(swapchainImageViews[i]);

            ASSERT(!framebufferExtent || framebufferExtent.value() == swapchainExtent);
            framebufferExtent = swapchainExtent;
         }
         else
         {
            ASSERT(resolveInfo.view);
            attachments.push_back(resolveInfo.view);

            ASSERT(!framebufferExtent || framebufferExtent.value() == resolveInfo.extent);
            framebufferExtent = resolveInfo.extent;
         }
      }

      ASSERT(framebufferExtent.has_value());
      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachments(attachments)
         .setWidth(framebufferExtent->width)
         .setHeight(framebufferExtent->height)
         .setLayers(1);

      ASSERT(!framebuffers[i]);
      framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
   }
}

void RenderPass::terminateRenderPass()
{
   if (renderPass)
   {
      device.destroyRenderPass(renderPass);
      renderPass = nullptr;
   }
}

void RenderPass::terminateFramebuffers()
{
   for (vk::Framebuffer framebuffer : framebuffers)
   {
      if (framebuffer)
      {
         device.destroyFramebuffer(framebuffer);
      }
   }
   framebuffers.clear();
}

void RenderPass::terminatePipelines()
{
   for (vk::Pipeline& pipeline : pipelines)
   {
      if (pipeline)
      {
         device.destroyPipeline(pipeline);
         pipeline = nullptr;
      }
   }

   if (pipelineLayout)
   {
      device.destroyPipelineLayout(pipelineLayout);
      pipelineLayout = nullptr;
   }
}

vk::Framebuffer RenderPass::getCurrentFramebuffer() const
{
   return framebuffers[hasSwapchainAttachment ? context.getSwapchainIndex() : 0];
}
