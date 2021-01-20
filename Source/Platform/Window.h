#pragma once

#include "Graphics/Vulkan.h"

#include "Platform/InputManager.h"

struct GLFWwindow;

struct WindowBounds
{
   int x = 0;
   int y = 0;
   int width = 0;
   int height = 0;
};

class Window
{
public:
   using FramebufferSizeChangedDelegate = Delegate<void, int, int>;
   using WindowRefreshRequestedDelegate = Delegate<void>;
   using WindowFocusDelegate = Delegate<void, bool>;

   Window();

   ~Window();

   void pollEvents();
   void waitEvents();

   bool shouldClose() const;
   void setTitle(const char* title);
   void toggleFullscreen();

   vk::SurfaceKHR createSurface(vk::Instance instance);
   vk::Extent2D getExtent() const;

   InputManager& getInputManager()
   {
      return inputManager;
   }

   void setCanConsumeCursorInput(bool consume);
   void releaseCursor();

   DelegateHandle bindOnFramebufferSizeChanged(FramebufferSizeChangedDelegate::FuncType&& function);
   void unbindOnFramebufferSizeChanged();

   DelegateHandle bindOnWindowRefreshRequested(WindowRefreshRequestedDelegate::FuncType&& function);
   void unbindOnWindowRefreshRequested();

   DelegateHandle bindOnWindowFocusChanged(WindowFocusDelegate::FuncType&& func);
   void unbindOnWindowFocusChanged();

private:
   friend class WindowCallbackHelper;

   void onFramebufferSizeChanged(int width, int height);
   void onWindowRefreshRequested();
   void onWindowFocusChanged(bool focused);
   void onKeyEvent(int key, int scancode, int action, int mods);
   void onMouseButtonEvent(int button, int action, int mods);
   void onCursorPosChanged(double xPos, double yPos);

   void setConsumeCursorInput(bool consume);

   GLFWwindow* glfwWindow = nullptr;

   InputManager inputManager;
   bool hasFocus = false;
   bool consumeCursorInput = false;
   bool canConsumeCursorInput = true;

   WindowBounds savedWindowBounds;

   FramebufferSizeChangedDelegate framebufferSizeChangedDelegate;
   WindowRefreshRequestedDelegate windowRefreshRequestedDelegate;
   WindowFocusDelegate windowFocusChangedDelegate;
};
