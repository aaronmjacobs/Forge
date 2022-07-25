#include "Renderer/Passes/UI/UIPass.h"

#include "Graphics/Command.h"
#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include <utility>

namespace
{
   void checkUiError(VkResult error)
   {
      ASSERT(error == VK_SUCCESS);
   }
}

UIPass::UIPass(const GraphicsContext& graphicsContext)
   : RenderPass(graphicsContext)
{
   std::array<vk::DescriptorPoolSize, 11> poolSizes =
   {
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eSampler).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eSampledImage).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageImage).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eUniformTexelBuffer).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageTexelBuffer).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageBuffer).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eUniformBufferDynamic).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageBufferDynamic).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight),
      vk::DescriptorPoolSize().setType(vk::DescriptorType::eInputAttachment).setDescriptorCount(1000 * GraphicsContext::kMaxFramesInFlight)
   };

   vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
      .setPoolSizes(poolSizes)
      .setMaxSets(1000 * GraphicsContext::kMaxFramesInFlight);

   descriptorPool = device.createDescriptorPool(poolCreateInfo);
}

UIPass::~UIPass()
{
   terminateImgui();
   context.delayedDestroy(std::move(descriptorPool));
   terminateFramebuffer();
   terminateRenderPass();
}

void UIPass::render(vk::CommandBuffer commandBuffer, Texture& uiTexture)
{
   SCOPED_LABEL(getName());

   uiTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   vk::Rect2D renderArea(vk::Offset2D(0, 0), uiTexture.getExtent());

   vk::ClearValue clearValue(std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f });

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(renderPass)
      .setFramebuffer(framebuffer)
      .setRenderArea(renderArea)
      .setClearValueCount(1)
      .setPClearValues(&clearValue);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   setViewport(commandBuffer, renderArea);

   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

   commandBuffer.endRenderPass();
}

void UIPass::updateFramebuffer(const Texture& uiTexture)
{
   terminateFramebuffer();
   initializeFramebuffer(uiTexture);
}

void UIPass::postUpdateAttachmentFormats()
{
   terminateImgui();
   terminateRenderPass();

   initializeRenderPass();
   initializeImgui();
}

void UIPass::initializeRenderPass()
{
   ASSERT(!renderPass);
   ASSERT(getColorFormats().size() == 1);

   vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
      .setFormat(getColorFormats()[0])
      .setSamples(getSampleCount())
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

   vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
      .setAttachment(0)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

   vk::SubpassDescription subpassDescription = vk::SubpassDescription()
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setPColorAttachments(&colorAttachmentReference)
      .setColorAttachmentCount(1);

   vk::SubpassDependency subpassDependency = vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

   vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
      .setPAttachments(&colorAttachment)
      .setAttachmentCount(1)
      .setSubpasses(subpassDescription)
      .setPDependencies(&subpassDependency)
      .setDependencyCount(1);

   renderPass = device.createRenderPass(renderPassCreateInfo);
   NAME_CHILD(renderPass, "Render Pass");
}

void UIPass::terminateRenderPass()
{
   context.delayedDestroy(std::move(renderPass));
}

void UIPass::initializeFramebuffer(const Texture& uiTexture)
{
   vk::ImageView uiTextureView = uiTexture.getDefaultView();
   vk::Extent2D extent = uiTexture.getExtent();

   vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
      .setRenderPass(renderPass)
      .setPAttachments(&uiTextureView)
      .setAttachmentCount(1)
      .setWidth(extent.width)
      .setHeight(extent.height)
      .setLayers(1);

   ASSERT(!framebuffer);
   framebuffer = device.createFramebuffer(framebufferCreateInfo);
   NAME_CHILD(framebuffer, "Framebuffer");
}

void UIPass::terminateFramebuffer()
{
   context.delayedDestroy(std::move(framebuffer));
}

void UIPass::initializeImgui()
{
   if (!imguiInitialized)
   {
      ImGui_ImplVulkan_InitInfo initInfo = {};
      initInfo.Instance = context.getInstance();
      initInfo.PhysicalDevice = context.getPhysicalDevice();
      initInfo.Device = device;
      initInfo.QueueFamily = context.getQueueFamilyIndices().graphicsFamily;
      initInfo.Queue = context.getGraphicsQueue();
      initInfo.PipelineCache = nullptr;
      initInfo.DescriptorPool = descriptorPool;
      initInfo.Subpass = 0;
      initInfo.MinImageCount = context.getSwapchain().getMinImageCount();
      initInfo.ImageCount = context.getSwapchain().getImageCount();
      initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
      initInfo.Allocator = nullptr;
      initInfo.CheckVkResultFn = &checkUiError;

      ImGui_ImplVulkan_Init(&initInfo, renderPass);

      Command::executeSingle(context, [](vk::CommandBuffer commandBuffer)
      {
         ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
      });

      ImGui_ImplVulkan_DestroyFontUploadObjects();

      imguiInitialized = true;
   }
}

void UIPass::terminateImgui()
{
   if (imguiInitialized)
   {
      ImGui_ImplVulkan_Shutdown();

      imguiInitialized = false;
   }
}
