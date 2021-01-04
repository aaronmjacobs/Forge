#include "ForgeApplication.h"

#include "Core/Assert.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/Swapchain.h"

#include "Renderer/Renderer.h"

#include "Resources/ResourceManager.h"

#include "Platform/Window.h"

#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>

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
   if (framebufferSizeChanged)
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

   vk::ResultValue<uint32_t> swapchainIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   if (swapchainIndexResultValue.result == vk::Result::eErrorOutOfDateKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }

      swapchainIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   }
   if (swapchainIndexResultValue.result != vk::Result::eSuccess && swapchainIndexResultValue.result != vk::Result::eSuboptimalKHR)
   {
      throw std::runtime_error("Failed to acquire swapchain image");
   }
   uint32_t swapchainIndex = swapchainIndexResultValue.value;

   if (swapchainFences[swapchainIndex])
   {
      // If a previous frame is still using the swapchain image, wait for it to complete
      vk::Result imageWaitResult = device.waitForFences({ swapchainFences[swapchainIndex] }, true, UINT64_MAX);
      if (imageWaitResult != vk::Result::eSuccess)
      {
         throw std::runtime_error("Failed to wait for image fence");
      }
   }
   swapchainFences[swapchainIndex] = frameFences[frameIndex];

   context->setSwapchainIndex(swapchainIndex);
   context->setFrameIndex(frameIndex);

   vk::CommandBuffer commandBuffer = commandBuffers[swapchainIndex];
   {
      vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      commandBuffer.begin(commandBufferBeginInfo);

      renderer->render(commandBuffer);

      commandBuffer.end();
   }

   std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[frameIndex] };
   std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
   static_assert(waitSemaphores.size() == waitStages.size(), "Wait semaphores and wait stages must be parallel arrays");

   std::array<vk::Semaphore, 1> signalSemaphores = { renderFinishedSemaphores[frameIndex] };
   vk::SubmitInfo submitInfo = vk::SubmitInfo()
      .setWaitSemaphores(waitSemaphores)
      .setPWaitDstStageMask(waitStages.data())
      .setCommandBuffers(commandBuffer)
      .setSignalSemaphores(signalSemaphores);

   device.resetFences({ frameFences[frameIndex] });
   context->getGraphicsQueue().submit({ submitInfo }, frameFences[frameIndex]);

   vk::SwapchainKHR swapchainKHR = swapchain->getSwapchainKHR();
   vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
      .setWaitSemaphores(signalSemaphores)
      .setSwapchains(swapchainKHR)
      .setImageIndices(swapchainIndex);

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

   frameIndex = (frameIndex + 1) % GraphicsContext::kMaxFramesInFlight;
}

bool ForgeApplication::recreateSwapchain()
{
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

   context->getDevice().waitIdle();

   terminateCommandBuffers(true);
   terminateSwapchain();

   initializeSwapchain();
   renderer->onSwapchainRecreated();
   initializeCommandBuffers();

   framebufferSizeChanged = false;

   return true;
}

void ForgeApplication::initializeGlfw()
{
   if (!glfwInit())
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   window = std::make_unique<Window>();

   window->bindOnFramebufferSizeChanged([this](int width, int height)
   {
      framebufferSizeChanged = true;
   });

   window->bindOnWindowRefreshRequested([this]()
   {
      render();
   });

   {
      static const char* kToggleFullscreenAction = "ToggleFullscreen";

      std::array<KeyChord, 2> keys = { KeyChord(Key::F11), KeyChord(Key::Enter, KeyMod::Alt) };
      window->getInputManager().createButtonMapping(kToggleFullscreenAction, keys, {}, {});

      window->getInputManager().bindButtonMapping(kToggleFullscreenAction, [this](bool pressed)
      {
         if (pressed)
         {
            window->toggleFullscreen();
         }
      });
   }
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
   resourceManager = std::make_unique<ResourceManager>(*context);
}

void ForgeApplication::terminateVulkan()
{
   resourceManager = nullptr;
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
   renderer = std::make_unique<Renderer>(*context, *resourceManager);
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
         .setQueueFamilyIndex(context->getQueueFamilyIndices().graphicsFamily)
         .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);

      commandPool = context->getDevice().createCommandPool(commandPoolCreateInfo);
   }

   vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
      .setCommandPool(commandPool)
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(GraphicsContext::kMaxFramesInFlight);

   ASSERT(commandBuffers.empty());
   commandBuffers = context->getDevice().allocateCommandBuffers(commandBufferAllocateInfo);
}

void ForgeApplication::terminateCommandBuffers(bool keepPoolAlive)
{
   if (commandPool)
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
   }

   commandBuffers.clear();
}

void ForgeApplication::initializeSyncObjects()
{
   imageAvailableSemaphores.resize(GraphicsContext::kMaxFramesInFlight);
   renderFinishedSemaphores.resize(GraphicsContext::kMaxFramesInFlight);
   frameFences.resize(GraphicsContext::kMaxFramesInFlight);
   swapchainFences.resize(swapchain->getImageCount());

   vk::SemaphoreCreateInfo semaphoreCreateInfo;
   vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo()
      .setFlags(vk::FenceCreateFlagBits::eSignaled);

   for (uint32_t i = 0; i < GraphicsContext::kMaxFramesInFlight; ++i)
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
