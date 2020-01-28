#pragma once

#include <vulkan/vulkan.hpp>

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

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
