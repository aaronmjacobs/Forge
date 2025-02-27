#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Graphics/Vulkan.h"

#include "Renderer/RenderSettings.h"

#include "UI/UI.h"

#include <memory>

class Renderer;
class ResourceManager;
class Scene;
class Swapchain;
class UI;
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

   void updateRenderSettings(const RenderSettings& newRenderSettings);

   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeSwapchain();
   void terminateSwapchain();

   void initializeRenderer();
   void terminateRenderer();

   void initializeUI();
   void terminateUI();

   void initializeCommandBuffers();
   void terminateCommandBuffers(bool keepPoolAlive);

   void initializeSyncObjects();
   void terminateSyncObjects();

   void loadScene();
   void unloadScene();

   RenderCapabilities renderCapabilities;
   RenderSettings renderSettings;

   std::unique_ptr<ResourceManager> resourceManager;

   std::unique_ptr<Window> window;
   std::unique_ptr<GraphicsContext> context;
   std::unique_ptr<Swapchain> swapchain;

   std::unique_ptr<Scene> scene;
   std::unique_ptr<Renderer> renderer;
   std::unique_ptr<UI> ui;

   vk::CommandPool commandPool;
   std::vector<vk::CommandBuffer> commandBuffers;

   std::vector<vk::Semaphore> imageAvailableSemaphores;
   std::vector<vk::Semaphore> renderFinishedSemaphores;
   std::vector<vk::Fence> frameFences;
   std::vector<vk::Fence> swapchainFences;
   uint32_t frameIndex = 0;

   bool framebufferSizeChanged = false;
};
