#include "ForgeApplication.h"

#include "Graphics/Renderer.h"
#include "Graphics/Swapchain.h"

#include "Platform/Window.h"

#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>

namespace
{
   const std::size_t kMaxFramesInFlight = 2;
}

ForgeApplication::ForgeApplication()
{
   initializeGlfw();
   initializeVulkan();
   initializeSwapchain();
   initializeRenderer();
   initializeCommandBuffers();
   initializeSyncObjects();
}

ForgeApplication::~ForgeApplication()
{
   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateRenderer();
   terminateSwapchain();
   terminateVulkan();
   terminateGlfw();
}

void ForgeApplication::run()
{
   while (!window->shouldClose())
   {
      window->pollEvents();
      render();
   }

   context->getDevice().waitIdle();
}

void ForgeApplication::render()
{
   if (window->pollFramebufferResized())
   {
      if (!recreateSwapchain())
      {
         return;
      }
   }

   vk::Device device = context->getDevice();

   vk::Result frameWaitResult = device.waitForFences({ frameFences[frameIndex] }, true, UINT64_MAX);
   if (frameWaitResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to wait for frame fence");
   }

   vk::ResultValue<uint32_t> imageIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   if (imageIndexResultValue.result == vk::Result::eErrorOutOfDateKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }

      imageIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   }
   if (imageIndexResultValue.result != vk::Result::eSuccess && imageIndexResultValue.result != vk::Result::eSuboptimalKHR)
   {
      throw std::runtime_error("Failed to acquire swapchain image");
   }
   uint32_t imageIndex = imageIndexResultValue.value;

   if (imageFences[imageIndex])
   {
      // If a previous frame is still using the image, wait for it to complete
      vk::Result imageWaitResult = device.waitForFences({ imageFences[imageIndex] }, true, UINT64_MAX);
      if (imageWaitResult != vk::Result::eSuccess)
      {
         throw std::runtime_error("Failed to wait for image fence");
      }
   }
   imageFences[imageIndex] = frameFences[frameIndex];

   std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[frameIndex] };
   std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
   static_assert(waitSemaphores.size() == waitStages.size(), "Wait semaphores and wait stages must be parallel arrays");

   updateUniformBuffers(imageIndex);

   std::array<vk::Semaphore, 1> signalSemaphores = { renderFinishedSemaphores[frameIndex] };
   vk::SubmitInfo submitInfo = vk::SubmitInfo()
      .setWaitSemaphores(waitSemaphores)
      .setPWaitDstStageMask(waitStages.data())
      .setCommandBuffers(commandBuffers[imageIndex])
      .setSignalSemaphores(signalSemaphores);

   device.resetFences({ frameFences[frameIndex] });
   context->getGraphicsQueue().submit({ submitInfo }, frameFences[frameIndex]);

   vk::SwapchainKHR swapchainKHR = swapchain->getSwapchainKHR();
   vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
      .setWaitSemaphores(signalSemaphores)
      .setSwapchains(swapchainKHR)
      .setImageIndices(imageIndex);

   vk::Result presentResult = context->getPresentQueue().presentKHR(presentInfo);
   if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }
   }
   else if (presentResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to present swapchain image");
   }

   frameIndex = (frameIndex + 1) % kMaxFramesInFlight;
}

void ForgeApplication::updateUniformBuffers(uint32_t index)
{
   renderer->updateUniformBuffers(index);
}

bool ForgeApplication::recreateSwapchain()
{
   context->getDevice().waitIdle();

   uint32_t oldSwapchainImageCount = swapchain->getImageCount();

   terminateCommandBuffers(true);
   terminateSwapchain();

   vk::Extent2D windowExtent = window->getExtent();
   while ((windowExtent.width == 0 || windowExtent.height == 0) && !window->shouldClose())
   {
      window->waitEvents();
      windowExtent = window->getExtent();
   }

   if (windowExtent.width == 0 || windowExtent.height == 0)
   {
      return false;
   }

   initializeSwapchain();

   uint32_t newSwapchainImageCount = swapchain->getImageCount();
   if (newSwapchainImageCount != oldSwapchainImageCount)
   {
      throw std::runtime_error("Swapchain image count changed unexpectedly");
   }

   renderer->onSwapchainRecreated();
   initializeCommandBuffers();

   return true;
}

void ForgeApplication::initializeGlfw()
{
   if (!glfwInit())
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   window = std::make_unique<Window>();
}

void ForgeApplication::terminateGlfw()
{
   ASSERT(window);
   window = nullptr;

   glfwTerminate();
}

void ForgeApplication::initializeVulkan()
{
   context = std::make_unique<GraphicsContext>(*window);
}

void ForgeApplication::terminateVulkan()
{
   resourceManager.clearAll();

   context = nullptr;
}

void ForgeApplication::initializeSwapchain()
{
   swapchain = std::make_unique<Swapchain>(*context, window->getExtent());
   context->setSwapchain(swapchain.get());
}

void ForgeApplication::terminateSwapchain()
{
   context->setSwapchain(nullptr);
   swapchain = nullptr;
}

void ForgeApplication::initializeRenderer()
{
   renderer = std::make_unique<Renderer>(*context, resourceManager);
}

void ForgeApplication::terminateRenderer()
{
   renderer = nullptr;
}

void ForgeApplication::initializeCommandBuffers()
{
   if (!commandPool)
   {
      vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
         .setQueueFamilyIndex(context->getQueueFamilyIndices().graphicsFamily);

      commandPool = context->getDevice().createCommandPool(commandPoolCreateInfo);
   }

   vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
      .setCommandPool(commandPool)
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(swapchain->getImageCount());

   ASSERT(commandBuffers.empty());
   commandBuffers = context->getDevice().allocateCommandBuffers(commandBufferAllocateInfo);

   for (std::size_t i = 0; i < commandBuffers.size(); ++i)
   {
      vk::CommandBuffer commandBuffer = commandBuffers[i];

      renderer->render(commandBuffer, static_cast<uint32_t>(i));

      commandBuffer.end();
   }
}

void ForgeApplication::terminateCommandBuffers(bool keepPoolAlive)
{
   if (keepPoolAlive)
   {
      context->getDevice().freeCommandBuffers(commandPool, commandBuffers);
   }
   else
   {
      // Command buffers will get cleaned up with the pool
      context->getDevice().destroyCommandPool(commandPool);
      commandPool = nullptr;
   }

   commandBuffers.clear();
}

void ForgeApplication::initializeSyncObjects()
{
   imageAvailableSemaphores.resize(kMaxFramesInFlight);
   renderFinishedSemaphores.resize(kMaxFramesInFlight);
   frameFences.resize(kMaxFramesInFlight);
   imageFences.resize(swapchain->getImageCount());

   vk::SemaphoreCreateInfo semaphoreCreateInfo;
   vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo()
      .setFlags(vk::FenceCreateFlagBits::eSignaled);

   for (std::size_t i = 0; i < kMaxFramesInFlight; ++i)
   {
      imageAvailableSemaphores[i] = context->getDevice().createSemaphore(semaphoreCreateInfo);
      renderFinishedSemaphores[i] = context->getDevice().createSemaphore(semaphoreCreateInfo);
      frameFences[i] = context->getDevice().createFence(fenceCreateInfo);
   }
}

void ForgeApplication::terminateSyncObjects()
{
   for (vk::Fence frameFence : frameFences)
   {
      context->getDevice().destroyFence(frameFence);
   }
   frameFences.clear();

   for (vk::Semaphore renderFinishedSemaphore : renderFinishedSemaphores)
   {
      context->getDevice().destroySemaphore(renderFinishedSemaphore);
   }
   renderFinishedSemaphores.clear();

   for (vk::Semaphore imageAvailableSemaphore : imageAvailableSemaphores)
   {
      context->getDevice().destroySemaphore(imageAvailableSemaphore);
   }
   imageAvailableSemaphores.clear();
}
