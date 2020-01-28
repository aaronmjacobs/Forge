#include "ForgeApplication.h"

#include "Core/Assert.h"
#include "Core/Log.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
#if FORGE_DEBUG
   VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
   {
      const char* typeName = nullptr;
      switch (messageType)
      {
      case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
         typeName = "general";
         break;
      case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
         typeName = "validation";
         break;
      case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
         typeName = "performance";
         break;
      default:
         ASSERT(false);
         typeName = "unknown";
         break;
      }

      const char* messageText = pCallbackData ? pCallbackData->pMessage : "none";
      std::string message = std::string("Vulkan debug message (type = ") + typeName + "): " + messageText;

      switch (messageSeverity)
      {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
         LOG_DEBUG(message);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
         LOG_INFO(message);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
         LOG_WARNING(message);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
         ASSERT(false, "%s", message.c_str());
         break;
      default:
         ASSERT(false);
         break;
      }

      return VK_FALSE;
   }

   VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo()
   {
      return vk::DebugUtilsMessengerCreateInfoEXT()
         .setPfnUserCallback(vulkanDebugMessageCallback)
         .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
         .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
   }

   VkDebugUtilsMessengerEXT createDebugMessenger(vk::Instance instance)
   {
      VkDebugUtilsMessengerEXT debugMessenger;
      if (PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(glfwGetInstanceProcAddress(instance, "vkCreateDebugUtilsMessengerEXT")))
      {
         VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = createDebugMessengerCreateInfo();
         if (pfnCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
         {
            ASSERT(false);
         }
      }

      return debugMessenger;
   }

   void destroyDebugMessenger(vk::Instance instance, VkDebugUtilsMessengerEXT& debugMessenger)
   {
      if (PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(glfwGetInstanceProcAddress(instance, "vkDestroyDebugUtilsMessengerEXT")))
      {
         pfnDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
         debugMessenger = nullptr;
      }
   }
#endif // FORGE_DEBUG

   std::vector<const char*> getExtensions()
   {
      std::vector<const char*> extensions;

      uint32_t glfwRequiredExtensionCount = 0;
      const char** glfwRequiredExtensionNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);

      extensions.reserve(glfwRequiredExtensionCount);
      for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
      {
         extensions.push_back(glfwRequiredExtensionNames[i]);
      }

#if FORGE_DEBUG
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // FORGE_DEBUG

      return extensions;
   }

   std::vector<const char*> getLayers()
   {
      std::vector<const char*> layers;

#if FORGE_DEBUG
      static const std::array<const char*, 2> kDesiredValidationLayers =
      {
         "VK_LAYER_KHRONOS_validation",
         "VK_LAYER_LUNARG_standard_validation"
      };

      std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
      for (const char* validationLayer : kDesiredValidationLayers)
      {
         auto location = std::find_if(layerProperties.begin(), layerProperties.end(), [validationLayer](const vk::LayerProperties& properties)
         {
            return std::strcmp(validationLayer, properties.layerName) == 0;
         });

         if (location != layerProperties.end())
         {
            layers.push_back(validationLayer);
         }
      }
#endif // FORGE_DEBUG

      return layers;
   }
}

ForgeApplication::ForgeApplication()
{
   if (!glfwInit())
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   if (!glfwVulkanSupported())
   {
      throw std::runtime_error("Vulkan is not supported on this machine");
   }

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   window = glfwCreateWindow(1280, 720, FORGE_PROJECT_NAME, nullptr, nullptr);

   vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
      .setPApplicationName(FORGE_PROJECT_NAME)
      .setApplicationVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setPEngineName(FORGE_PROJECT_NAME)
      .setEngineVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setApiVersion(VK_API_VERSION_1_1);

#if FORGE_DEBUG
   VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = createDebugMessengerCreateInfo();
   applicationInfo = applicationInfo.setPNext(&debugUtilsMessengerCreateInfo);
#endif // FORGE_DEBUG

   std::vector<const char*> extensions = getExtensions();
   std::vector<const char*> layers = getLayers();

   vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
      .setPApplicationInfo(&applicationInfo)
      .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
      .setPpEnabledExtensionNames(extensions.data())
      .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
      .setPpEnabledLayerNames(layers.data());

   instance = vk::createInstance(createInfo);

#if FORGE_DEBUG
   debugMessenger = createDebugMessenger(instance);
#endif // FORGE_DEBUG
}

ForgeApplication::~ForgeApplication()
{
#if FORGE_DEBUG
   destroyDebugMessenger(instance, debugMessenger);
#endif // FORGE_DEBUG

   instance.destroy();
   instance = nullptr;

   glfwDestroyWindow(window);
   window = nullptr;

   glfwTerminate();
}

void ForgeApplication::run()
{
   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();
   }
}
