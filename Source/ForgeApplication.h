#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

struct GLFWwindow;

class ForgeApplication
{
public:
   ForgeApplication();
   ~ForgeApplication();

   void run();

private:
   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeRenderPass();
   void terminateRenderPass();

   void initializeGraphicsPipeline();
   void terminateGraphicsPipeline();

   void initializeFramebuffers();
   void terminateFramebuffers();

   GLFWwindow* window = nullptr;

   vk::Instance instance;
   vk::SurfaceKHR surface;
   vk::Device device;
   vk::Queue graphicsQueue;
   vk::Queue presentQueue;

   vk::SwapchainKHR swapchain;
   std::vector<vk::Image> swapchainImages;
   vk::Format swapchainImageFormat;
   vk::Extent2D swapchainExtent;
   std::vector<vk::ImageView> swapchainImageViews;

   vk::RenderPass renderPass;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
