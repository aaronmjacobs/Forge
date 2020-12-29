#pragma once

#include "Graphics/GraphicsContext.h"

#include "Resources/ResourceManager.h"

#include <memory>

class Renderer;
class SimpleRenderPass;
class Swapchain;
class Window;

class ForgeApplication
{
public:
   ForgeApplication();
   ~ForgeApplication();

   void run();

private:
   void render();

   bool recreateSwapchain();

   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeSwapchain();
   void terminateSwapchain();

   void initializeRenderer();
   void terminateRenderer();

   void initializeCommandBuffers();
   void terminateCommandBuffers(bool keepPoolAlive);

   void initializeSyncObjects();
   void terminateSyncObjects();

   ResourceManager resourceManager;

   std::unique_ptr<Window> window;
   std::unique_ptr<GraphicsContext> context;
   std::unique_ptr<Swapchain> swapchain;

   std::unique_ptr<Renderer> renderer;

   vk::CommandPool commandPool;
   std::vector<vk::CommandBuffer> commandBuffers;

   std::vector<vk::Semaphore> imageAvailableSemaphores;
   std::vector<vk::Semaphore> renderFinishedSemaphores;
   std::vector<vk::Fence> frameFences;
   std::vector<vk::Fence> imageFences;
   std::size_t frameIndex = 0;
};
