#include "ForgeApplication.h"

#include "Core/Assert.h"
#include "Core/Log.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <optional>
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
      VkDebugUtilsMessengerEXT debugMessenger = nullptr;
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

   struct QueueFamilyIndices
   {
      std::optional<uint32_t> graphicsFamily;
   };

   QueueFamilyIndices getQueueFamilyIndices(vk::PhysicalDevice physicalDevice)
   {
      QueueFamilyIndices queueFamilyIndices;

      uint32_t index = 0;
      for (vk::QueueFamilyProperties queueFamilyProperties : physicalDevice.getQueueFamilyProperties())
      {
         if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
         {
            queueFamilyIndices.graphicsFamily = index;
         }

         ++index;
      }

      return queueFamilyIndices;
   }

   int getPhysicalDeviceScore(vk::PhysicalDevice physicalDevice)
   {
      QueueFamilyIndices queueFamilyIndices = getQueueFamilyIndices(physicalDevice);
      if (!queueFamilyIndices.graphicsFamily)
      {
         return -1;
      }

      int score = 0;

      vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
      switch (properties.deviceType)
      {
      case vk::PhysicalDeviceType::eOther:
         break;
      case vk::PhysicalDeviceType::eIntegratedGpu:
         score += 100;
         break;
      case vk::PhysicalDeviceType::eDiscreteGpu:
         score += 1000;
         break;
      case vk::PhysicalDeviceType::eVirtualGpu:
         score += 10;
         break;
      case vk::PhysicalDeviceType::eCpu:
         score += 1;
         break;
      }

      vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();
      if (features.robustBufferAccess)
      {
         score += 1;
      }

      return score;
   }

   vk::PhysicalDevice selectBestPhysicalDevice(const std::vector<vk::PhysicalDevice>& devices)
   {
      std::multimap<int, vk::PhysicalDevice> devicesByScore;
      for (vk::PhysicalDevice device : devices)
      {
         devicesByScore.emplace(getPhysicalDeviceScore(device), device);
      }

      vk::PhysicalDevice bestPhysicalDevice;
      if (!devicesByScore.empty() && devicesByScore.rbegin()->first > 0)
      {
         bestPhysicalDevice = devicesByScore.rbegin()->second;
      }

      return bestPhysicalDevice;
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

   vk::PhysicalDevice physicalDevice = selectBestPhysicalDevice(instance.enumeratePhysicalDevices());
   if (!physicalDevice)
   {
      throw std::runtime_error("Failed to find a suitable GPU");
   }

   QueueFamilyIndices queueFamilyIndices = getQueueFamilyIndices(physicalDevice);
   ASSERT(queueFamilyIndices.graphicsFamily);

   float queuePriority = 1.0f;
   vk::DeviceQueueCreateInfo deviceQueueCreateInfo = vk::DeviceQueueCreateInfo()
      .setQueueFamilyIndex(*queueFamilyIndices.graphicsFamily)
      .setQueueCount(1)
      .setPQueuePriorities(&queuePriority);

   vk::PhysicalDeviceFeatures deviceFeatures;

   vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
      .setPQueueCreateInfos(&deviceQueueCreateInfo)
      .setQueueCreateInfoCount(1)
      .setPEnabledFeatures(&deviceFeatures);

#if FORGE_DEBUG
   deviceCreateInfo = deviceCreateInfo
      .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
      .setPpEnabledLayerNames(layers.data());
#endif // FORGE_DEBUG

   device = physicalDevice.createDevice(deviceCreateInfo);
   graphicsQueue = device.getQueue(*queueFamilyIndices.graphicsFamily, 0);
}

ForgeApplication::~ForgeApplication()
{
#if FORGE_DEBUG
   destroyDebugMessenger(instance, debugMessenger);
#endif // FORGE_DEBUG

   device.destroy();

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
