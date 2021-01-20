#include "Platform/Window.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>

namespace
{
   const int kInitialWindowWidth = 1280;
   const int kInitialWindowHeight = 720;

   GLFWmonitor* selectFullScreenMonitor(const WindowBounds& windowBounds)
   {
      GLFWmonitor* fullScreenMonitor = glfwGetPrimaryMonitor();

      int windowCenterX = windowBounds.x + (windowBounds.width / 2);
      int windowCenterY = windowBounds.y + (windowBounds.height / 2);

      int monitorCount = 0;
      GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

      for (int i = 0; i < monitorCount; ++i)
      {
         GLFWmonitor* candidateMonitor = monitors[i];

         if (const GLFWvidmode* vidMode = glfwGetVideoMode(candidateMonitor))
         {
            WindowBounds monitorBounds;
            glfwGetMonitorPos(candidateMonitor, &monitorBounds.x, &monitorBounds.y);
            monitorBounds.width = vidMode->width;
            monitorBounds.height = vidMode->height;

            if (windowCenterX >= monitorBounds.x && windowCenterX < monitorBounds.x + monitorBounds.width
               && windowCenterY >= monitorBounds.y && windowCenterY < monitorBounds.y + monitorBounds.height)
            {
               fullScreenMonitor = candidateMonitor;
               break;
            }
         }
      }

      return fullScreenMonitor;
   }
}

class WindowCallbackHelper
{
public:
   static void framebufferSizeCallback(GLFWwindow* glfwWindow, int width, int height);
   static void windowRefreshCallback(GLFWwindow* glfwWindow);
   static void windowFocusCallback(GLFWwindow* glfwWindow, int focused);
   static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
   static void cursorPosCallback(GLFWwindow* glfwWindow, double xPos, double yPos);
   static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
};

// static
void WindowCallbackHelper::framebufferSizeCallback(GLFWwindow* glfwWindow, int width, int height)
{
   Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
   ASSERT(window);

   window->onFramebufferSizeChanged(width, height);
}

// static
void WindowCallbackHelper::windowRefreshCallback(GLFWwindow* glfwWindow)
{
   Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
   ASSERT(window);

   window->onWindowRefreshRequested();
}

// static
void WindowCallbackHelper::windowFocusCallback(GLFWwindow* glfwWindow, int focused)
{
   Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
   ASSERT(window);

   window->onWindowFocusChanged(focused == GLFW_TRUE);
}

// static
void WindowCallbackHelper::keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
   Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
   ASSERT(window);

   window->onKeyEvent(key, scancode, action, mods);
}

// static
void WindowCallbackHelper::mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods)
{
   Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
   ASSERT(window);

   window->onMouseButtonEvent(button, action, mods);
}

// static
void WindowCallbackHelper::cursorPosCallback(GLFWwindow* glfwWindow, double xPos, double yPos)
{
   Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
   ASSERT(window);

   window->onCursorPosChanged(xPos, yPos);
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

   setConsumeCursorInput(true);

   double cursorX = 0.0;
   double cursorY = 0.0;
   glfwGetCursorPos(glfwWindow, &cursorX, &cursorY);
   inputManager.init(cursorX, cursorY);

   // Clear out initial events before registering for callbacks
   pollEvents();

   glfwSetWindowUserPointer(glfwWindow, this);
   glfwSetFramebufferSizeCallback(glfwWindow, WindowCallbackHelper::framebufferSizeCallback);
   glfwSetWindowRefreshCallback(glfwWindow, WindowCallbackHelper::windowRefreshCallback);
   glfwSetWindowFocusCallback(glfwWindow, WindowCallbackHelper::windowFocusCallback);
   glfwSetKeyCallback(glfwWindow, WindowCallbackHelper::keyCallback);
   glfwSetMouseButtonCallback(glfwWindow, WindowCallbackHelper::mouseButtonCallback);
   glfwSetCursorPosCallback(glfwWindow, WindowCallbackHelper::cursorPosCallback);
}

Window::~Window()
{
   ASSERT(glfwWindow);
   glfwDestroyWindow(glfwWindow);
}

void Window::pollEvents()
{
   glfwPollEvents();
   inputManager.pollEvents();
}

void Window::waitEvents()
{
   glfwWaitEvents();
}

bool Window::shouldClose() const
{
   return glfwWindowShouldClose(glfwWindow);
}

void Window::setTitle(const char* title)
{
   glfwSetWindowTitle(glfwWindow, title);
}

void Window::toggleFullscreen()
{
   if (GLFWmonitor* currentMonitor = glfwGetWindowMonitor(glfwWindow))
   {
      // Currently in full screen mode, swap back to windowed (with last saved window location)
      glfwSetWindowMonitor(glfwWindow, nullptr, savedWindowBounds.x, savedWindowBounds.y, savedWindowBounds.width, savedWindowBounds.height, 0);
   }
   else
   {
      // Currently in windowed mode, save the window location and swap to full screen
      glfwGetWindowPos(glfwWindow, &savedWindowBounds.x, &savedWindowBounds.y);
      glfwGetWindowSize(glfwWindow, &savedWindowBounds.width, &savedWindowBounds.height);

      if (GLFWmonitor* newMonitor = selectFullScreenMonitor(savedWindowBounds))
      {
         if (const GLFWvidmode* vidMode = glfwGetVideoMode(newMonitor))
         {
            glfwSetWindowMonitor(glfwWindow, newMonitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
         }
      }
   }
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

void Window::setCanConsumeCursorInput(bool consume)
{
   canConsumeCursorInput = consume;

   if (!consume)
   {
      setConsumeCursorInput(consumeCursorInput);
   }
}

void Window::releaseCursor()
{
   setConsumeCursorInput(false);
}

DelegateHandle Window::bindOnFramebufferSizeChanged(FramebufferSizeChangedDelegate::FuncType&& function)
{
   return framebufferSizeChangedDelegate.bind(std::move(function));
}

void Window::unbindOnFramebufferSizeChanged()
{
   framebufferSizeChangedDelegate.unbind();
}

DelegateHandle Window::bindOnWindowRefreshRequested(WindowRefreshRequestedDelegate::FuncType&& function)
{
   return windowRefreshRequestedDelegate.bind(std::move(function));
}

void Window::unbindOnWindowRefreshRequested()
{
   windowRefreshRequestedDelegate.unbind();
}

DelegateHandle Window::bindOnWindowFocusChanged(WindowFocusDelegate::FuncType&& function)
{
   return windowFocusChangedDelegate.bind(std::move(function));
}

void Window::unbindOnWindowFocusChanged()
{
   windowFocusChangedDelegate.unbind();
}

void Window::onFramebufferSizeChanged(int width, int height)
{
   framebufferSizeChangedDelegate.executeIfBound(width, height);
}

void Window::onWindowRefreshRequested()
{
   windowRefreshRequestedDelegate.executeIfBound();
}

void Window::onWindowFocusChanged(bool focused)
{
   hasFocus = focused;

   if (focused)
   {
      double cursorX = 0.0;
      double cursorY = 0.0;
      glfwGetCursorPos(glfwWindow, &cursorX, &cursorY);

      int width = 0;
      int height = 0;
      glfwGetWindowSize(glfwWindow, &width, &height);

      // Ignore the event if the cursor isn't within the bounds of the window
      if (cursorX >= 0.0 && cursorX <= width && cursorY >= 0.0 && cursorY <= height)
      {
         setConsumeCursorInput(true);
      }
   }
   else
   {
      setConsumeCursorInput(false);
   }

   windowFocusChangedDelegate.executeIfBound(focused);
}

void Window::onKeyEvent(int key, int scancode, int action, int mods)
{
   inputManager.onKeyEvent(key, scancode, action, mods);
}

void Window::onMouseButtonEvent(int button, int action, int mods)
{
   inputManager.onMouseButtonEvent(button, action, mods);

   if (hasFocus && !consumeCursorInput)
   {
      setConsumeCursorInput(true);
   }
}

void Window::onCursorPosChanged(double xPos, double yPos)
{
   inputManager.onCursorPosChanged(xPos, yPos, consumeCursorInput);
}

void Window::setConsumeCursorInput(bool consume)
{
   consumeCursorInput = consume && canConsumeCursorInput;
   glfwSetInputMode(glfwWindow, GLFW_CURSOR, consumeCursorInput ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}
