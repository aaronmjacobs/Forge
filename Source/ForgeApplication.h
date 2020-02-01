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

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
