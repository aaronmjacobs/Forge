#include "Renderer/Passes/UI/UIPass.h"

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
      .setMaxSets(1000 * GraphicsContext::kMaxFramesInFlight)
      .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

   descriptorPool = device.createDescriptorPool(poolCreateInfo);
   NAME_CHILD(descriptorPool, "Descriptor Pool");
}

UIPass::~UIPass()
{
   terminateImgui();
   context.delayedDestroy(std::move(descriptorPool));
}

void UIPass::render(vk::CommandBuffer commandBuffer, Texture& outputTexture)
{
   SCOPED_LABEL(getName());

   outputTexture.transitionLayout(commandBuffer, TextureLayoutType::AttachmentWrite);

   ImGui_ImplVulkan_NewFrame();

   AttachmentInfo colorAttachmentInfo = AttachmentInfo(outputTexture)
      .setLoadOp(vk::AttachmentLoadOp::eClear);

   executePass(commandBuffer, &colorAttachmentInfo, nullptr, [](vk::CommandBuffer commandBuffer)
   {
      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, nullptr);
   });
}

void UIPass::onOutputTextureCreated(const Texture& outputTexture)
{
   vk::Format format = outputTexture.getImageProperties().format;
   vk::SampleCountFlagBits sampleCount = outputTexture.getTextureProperties().sampleCount;

   if (!imguiInitialized || format != cachedFormat || sampleCount != cachedSampleCount)
   {
      terminateImgui();
      initializeImgui(format, sampleCount);
   }
}

void UIPass::initializeImgui(vk::Format format, vk::SampleCountFlagBits sampleCount)
{
   if (!imguiInitialized)
   {
      ImGui_ImplVulkan_InitInfo initInfo = {};
      initInfo.Instance = context.getInstance();
      initInfo.PhysicalDevice = context.getPhysicalDevice();
      initInfo.Device = device;
      initInfo.QueueFamily = context.getGraphicsFamilyIndex();
      initInfo.Queue = context.getGraphicsQueue();
      initInfo.DescriptorPool = descriptorPool;
      initInfo.RenderPass = nullptr;
      initInfo.MinImageCount = context.getSwapchain().getMinImageCount();
      initInfo.ImageCount = context.getSwapchain().getImageCount();
      initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(sampleCount);

      initInfo.PipelineCache = context.getPipelineCache();
      initInfo.Subpass = 0;

      initInfo.UseDynamicRendering = true;
      initInfo.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo()
         .setColorAttachmentFormats(format);

      initInfo.Allocator = nullptr;
      initInfo.CheckVkResultFn = &checkUiError;
      initInfo.MinAllocationSize = 0;

      ImGui_ImplVulkan_Init(&initInfo);

      imguiInitialized = true;
      cachedFormat = format;
      cachedSampleCount = sampleCount;
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
