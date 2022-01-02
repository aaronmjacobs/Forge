#include "Renderer/Passes/UI/UIPass.h"

#include "Graphics/Command.h"
#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"

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
   clearColor = true;

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
}

void UIPass::render(vk::CommandBuffer commandBuffer, FramebufferHandle framebufferHandle)
{
   SCOPED_LABEL(getName());

   const Framebuffer* framebuffer = getFramebuffer(framebufferHandle);
   if (!framebuffer)
   {
      ASSERT(false);
      return;
   }

   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 0.0f };
   std::array<vk::ClearValue, 1> clearValues = { vk::ClearColorValue(clearColorValues) };

   beginRenderPass(commandBuffer, *framebuffer, clearValues);

   ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

   endRenderPass(commandBuffer);
}

void UIPass::initializePipelines(vk::SampleCountFlagBits sampleCount)
{
   terminateImgui();
   initializeImgui();
}

std::vector<vk::SubpassDependency> UIPass::getSubpassDependencies() const
{
   std::vector<vk::SubpassDependency> subpassDependencies;

   subpassDependencies.push_back(vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite));

   return subpassDependencies;
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

      ImGui_ImplVulkan_Init(&initInfo, getRenderPass());

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
