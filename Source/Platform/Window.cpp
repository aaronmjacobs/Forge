#include "Platform/Window.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>

namespace
{
   const int kInitialWindowWidth = 1280;
   const int kInitialWindowHeight = 720;

   void framebufferSizeCallback(GLFWwindow* glfwWindow, int width, int height)
   {
      if (Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow)))
      {
         window->onFramebufferResized();
      }
   }
}

Window::Window()
{
   if (!glfwVulkanSupported())
   {
      throw std::runtime_error("Vulkan is not supported on this machine");
   }

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindow = glfwCreateWindow(kInitialWindowWidth, kInitialWindowHeight, FORGE_PROJECT_NAME, nullptr, nullptr);
   if (!glfwWindow)
   {
      throw std::runtime_error("Failed to create window");
   }

   glfwSetWindowUserPointer(glfwWindow, this);
   glfwSetFramebufferSizeCallback(glfwWindow, framebufferSizeCallback);
}

Window::~Window()
{
   ASSERT(glfwWindow);
   glfwDestroyWindow(glfwWindow);
}

void Window::pollEvents()
{
   glfwPollEvents();
}

void Window::waitEvents()
{
   glfwWaitEvents();
}

bool Window::shouldClose() const
{
   return glfwWindowShouldClose(glfwWindow);
}

vk::SurfaceKHR Window::createSurface(vk::Instance instance)
{
   VkSurfaceKHR vkSurface = nullptr;
   if (glfwCreateWindowSurface(instance, glfwWindow, nullptr, &vkSurface) != VK_SUCCESS)
   {
      throw std::runtime_error("Failed to create window surface");
   }

   return vkSurface;
}

vk::Extent2D Window::getExtent() const
{
   int width = 0;
   int height = 0;
   glfwGetFramebufferSize(glfwWindow, &width, &height);

   return vk::Extent2D(static_cast<uint32_t>(std::max(width, 0)), static_cast<uint32_t>(std::max(height, 0)));
}

void Window::onFramebufferResized()
{
   framebufferResized = true;
}

bool Window::pollFramebufferResized()
{
   bool resized = framebufferResized;
   framebufferResized = false;

   return resized;
}
