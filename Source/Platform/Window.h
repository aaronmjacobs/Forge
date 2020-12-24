#pragma once

#include "Graphics/Vulkan.h"

struct GLFWwindow;

class Window
{
public:
   Window();

   ~Window();
   
   void pollEvents();
   void waitEvents();
   
   bool shouldClose() const;
   
   vk::SurfaceKHR createSurface(vk::Instance instance);
   vk::Extent2D getExtent() const;
   
   void onFramebufferResized();
   bool pollFramebufferResized();
   
private:
   GLFWwindow* glfwWindow = nullptr;
   bool framebufferResized = false;
};
