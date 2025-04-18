#include "Renderer/Passes/UI/UIPass.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include <array>

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
}

UIPass::~UIPass()
{
   terminateImgui();
}

void UIPass::render(vk::CommandBuffer commandBuffer, Texture& outputTexture)
{
   SCOPED_LABEL(getName());

   ASSERT(outputTexture.getImageProperties().format == cachedFormat && outputTexture.getTextureProperties().sampleCount == cachedSampleCount);

   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setClearValue(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }));

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [](vk::CommandBuffer commandBuffer)
   {
      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
   });
}

void UIPass::onOutputTextureCreated(const Texture& outputTexture)
{
   vk::Format format = outputTexture.getImageProperties().format;
   vk::SampleCountFlagBits sampleCount = outputTexture.getTextureProperties().sampleCount;

   if (format != cachedFormat || sampleCount != cachedSampleCount)
   {
      terminateImgui();
      initializeImgui(format, sampleCount);
   }
}

void UIPass::initializeImgui(vk::Format format, vk::SampleCountFlagBits sampleCount)
{
   ASSERT(!imguiInitialized);

   ImGui_ImplVulkan_InitInfo initInfo = {};
   initInfo.ApiVersion = GraphicsContext::kVulkanTargetVersion;
   initInfo.Instance = context.getInstance();
   initInfo.PhysicalDevice = context.getPhysicalDevice();
   initInfo.Device = device;
   initInfo.QueueFamily = context.getGraphicsFamilyIndex();
   initInfo.Queue = context.getGraphicsQueue();
   initInfo.MinImageCount = context.getSwapchain().getMinImageCount();
   initInfo.ImageCount = context.getSwapchain().getImageCount();
   initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(sampleCount);
   initInfo.PipelineCache = context.getPipelineCache();
   initInfo.DescriptorPoolSize = 1000 * GraphicsContext::kMaxFramesInFlight;
   initInfo.UseDynamicRendering = true;
   initInfo.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo().setColorAttachmentFormats(format);
   initInfo.CheckVkResultFn = &checkUiError;

   ImGui_ImplVulkan_Init(&initInfo);

   cachedFormat = format;
   cachedSampleCount = sampleCount;

   imguiInitialized = true;
}

void UIPass::terminateImgui()
{
   if (imguiInitialized)
   {
      ImGui_ImplVulkan_Shutdown();

      imguiInitialized = false;
   }
}
