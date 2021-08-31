#include "Graphics/GraphicsContext.h"

#include "Core/Log.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/DelayedObjectDestroyer.h"
#include "Graphics/Swapchain.h"

#include "Platform/Window.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
   bool hasExtensionProperty(const std::vector<vk::ExtensionProperties>& extensionProperties, const char* name)
   {
      return std::find_if(extensionProperties.begin(), extensionProperties.end(), [name](const vk::ExtensionProperties& properties)
      {
         return std::strcmp(name, properties.extensionName) == 0;
      }) != extensionProperties.end();
   }

   bool hasLayerProperty(const std::vector<vk::LayerProperties>& layerProperties, const char* name)
   {
      return std::find_if(layerProperties.begin(), layerProperties.end(), [name](const vk::LayerProperties& properties)
      {
         return std::strcmp(name, properties.layerName) == 0;
      }) != layerProperties.end();
   }

   std::vector<const char*> getExtensions(const Window& window)
   {
      std::vector<const char*> extensions;

      std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();
      for (const char* requiredExtension : window.getRequiredExtensions())
      {
         if (hasExtensionProperty(extensionProperties, requiredExtension))
         {
            extensions.push_back(requiredExtension);
         }
         else
         {
            throw std::runtime_error(std::string("Required extension was missing: ") + requiredExtension);
         }
      }

#if FORGE_DEBUG
      if (hasExtensionProperty(extensionProperties, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
      {
         extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
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
         if (hasLayerProperty(layerProperties, validationLayer))
         {
            layers.push_back(validationLayer);
         }
      }
#endif // FORGE_DEBUG

      return layers;
   }

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
#endif // FORGE_DEBUG

   std::vector<const char*> getDeviceExtensions(vk::PhysicalDevice physicalDevice)
   {
      static const std::array<const char*, 1> kRequiredDeviceExtensions =
      {
         VK_KHR_SWAPCHAIN_EXTENSION_NAME
      };

      static const std::array<const char*, 2> kOptionalDeviceExtensions =
      {
         VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, // For MoltenVK
         VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME
      };

      std::vector<const char*> deviceExtensions;
      deviceExtensions.reserve(kRequiredDeviceExtensions.size() + kOptionalDeviceExtensions.size());

      std::vector<vk::ExtensionProperties> deviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();

      for (const char* requiredDeviceExtension : kRequiredDeviceExtensions)
      {
         if (hasExtensionProperty(deviceExtensionProperties, requiredDeviceExtension))
         {
            deviceExtensions.push_back(requiredDeviceExtension);
         }
         else
         {
            throw std::runtime_error(std::string("Required device was missing: ") + requiredDeviceExtension);
         }
      }

      for (const char* optionalDeviceExtension : kOptionalDeviceExtensions)
      {
         if (hasExtensionProperty(deviceExtensionProperties, optionalDeviceExtension))
         {
            deviceExtensions.push_back(optionalDeviceExtension);
         }
      }

      return deviceExtensions;
   }

   int getPhysicalDeviceScore(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
   {
      std::vector<const char*> deviceExtensions;
      try
      {
         deviceExtensions = getDeviceExtensions(physicalDevice);
      }
      catch (const std::runtime_error&)
      {
         return -1;
      }

      SwapchainSupportDetails swapChainSupportDetails = Swapchain::getSupportDetails(physicalDevice, surface);
      if (!swapChainSupportDetails.isValid())
      {
         return -1;
      }

      std::optional<QueueFamilyIndices> queueFamilyIndices = QueueFamilyIndices::get(physicalDevice, surface);
      if (!queueFamilyIndices)
      {
         return -1;
      }

      vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

      if (properties.apiVersion >= VK_API_VERSION_1_2)
      {
         vk::PhysicalDeviceFeatures2 features2;
         vk::PhysicalDeviceVulkan12Features vulkan12Features;
         features2.setPNext(&vulkan12Features);
         physicalDevice.getFeatures2(&features2);

         if (!vulkan12Features.uniformBufferStandardLayout)
         {
            return -1;
         }
      }
      else
      {
         if (std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME) == deviceExtensions.end())
         {
            return -1;
         }
      }

      int score = 0;

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
      if (features.samplerAnisotropy)
      {
         score += 1;
      }
      if (features.robustBufferAccess)
      {
         score += 1;
      }

      return score;
   }

   vk::PhysicalDevice selectBestPhysicalDevice(const std::vector<vk::PhysicalDevice>& devices, vk::SurfaceKHR surface)
   {
      std::multimap<int, vk::PhysicalDevice> devicesByScore;
      for (vk::PhysicalDevice device : devices)
      {
         devicesByScore.emplace(getPhysicalDeviceScore(device, surface), device);
      }

      vk::PhysicalDevice bestPhysicalDevice;
      if (!devicesByScore.empty() && devicesByScore.rbegin()->first > 0)
      {
         bestPhysicalDevice = devicesByScore.rbegin()->second;
      }

      return bestPhysicalDevice;
   }
}

// static
std::optional<QueueFamilyIndices> QueueFamilyIndices::get(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
   std::optional<uint32_t> graphicsFamilyIndex;
   std::optional<uint32_t> presentFamilyIndex;

   uint32_t index = 0;
   for (const vk::QueueFamilyProperties& queueFamilyProperties : physicalDevice.getQueueFamilyProperties())
   {
      if (!graphicsFamilyIndex && (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics))
      {
         graphicsFamilyIndex = index;
      }

      if (!presentFamilyIndex && physicalDevice.getSurfaceSupportKHR(index, surface))
      {
         presentFamilyIndex = index;
      }

      if (graphicsFamilyIndex && presentFamilyIndex)
      {
         QueueFamilyIndices indices;
         indices.graphicsFamily = *graphicsFamilyIndex;
         indices.presentFamily = *presentFamilyIndex;

         return indices;
      }

      ++index;
   }

   return {};
}

// static
const vk::DispatchLoaderDynamic& GraphicsContext::GetDynamicLoader()
{
   return dispatchLoaderDynamic;
}

// static
vk::DispatchLoaderDynamic GraphicsContext::dispatchLoaderDynamic;

GraphicsContext::GraphicsContext(Window& window)
{
   vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
      .setPApplicationName(FORGE_PROJECT_NAME)
      .setApplicationVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setPEngineName(FORGE_PROJECT_NAME)
      .setEngineVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setApiVersion(VK_API_VERSION_1_2);

   std::vector<const char*> extensions = getExtensions(window);
   std::vector<const char*> layers = getLayers();

   vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
      .setPApplicationInfo(&applicationInfo)
      .setPEnabledExtensionNames(extensions)
      .setPEnabledLayerNames(layers);

#if FORGE_DEBUG
   VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = createDebugMessengerCreateInfo();
   createInfo.setPNext(&debugUtilsMessengerCreateInfo);
#endif // FORGE_DEBUG

   instance = vk::createInstance(createInfo);
   dispatchLoaderDynamic.init(instance, vk::Device());

#if FORGE_DEBUG
#  define FIND_VULKAN_FUNCTION(instance, name) reinterpret_cast<PFN_##name>(window.getInstanceProcAddress(instance, #name))
   pfnCreateDebugUtilsMessengerEXT = FIND_VULKAN_FUNCTION(instance, vkCreateDebugUtilsMessengerEXT);
   pfnDestroyDebugUtilsMessengerEXT = FIND_VULKAN_FUNCTION(instance, vkDestroyDebugUtilsMessengerEXT);
#  undef FIND_VULKAN_FUNCTION

   if (pfnCreateDebugUtilsMessengerEXT)
   {
      VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = createDebugMessengerCreateInfo();
      if (pfnCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
      {
         ASSERT(false);
      }
   }
#endif // FORGE_DEBUG

   surface = window.createSurface(instance);

   physicalDevice = selectBestPhysicalDevice(instance.enumeratePhysicalDevices(), surface);
   if (!physicalDevice)
   {
      throw std::runtime_error("Failed to find a suitable GPU");
   }
   physicalDeviceProperties = physicalDevice.getProperties();
   physicalDeviceFeatures = physicalDevice.getFeatures();

   std::optional<QueueFamilyIndices> potentialQueueFamilyIndices = QueueFamilyIndices::get(physicalDevice, surface);
   if (!potentialQueueFamilyIndices)
   {
      throw std::runtime_error("Failed to get queue family indices");
   }
   queueFamilyIndices = *potentialQueueFamilyIndices;
   std::set<uint32_t> uniqueQueueIndices = queueFamilyIndices.getUniqueIndices();

   float queuePriority = 1.0f;
   std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
   for (uint32_t queueFamilyIndex : uniqueQueueIndices)
   {
      deviceQueueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
         .setQueueFamilyIndex(queueFamilyIndex)
         .setQueuePriorities(queuePriority));
   }

   std::vector<const char*> deviceExtensions = getDeviceExtensions(physicalDevice);

   vk::PhysicalDeviceFeatures deviceFeatures;
   deviceFeatures.setSamplerAnisotropy(physicalDeviceFeatures.samplerAnisotropy);
   deviceFeatures.setSampleRateShading(true);
   deviceFeatures.setImageCubeArray(true);
   deviceFeatures.setDepthBiasClamp(true);

   void* deviceCreateInfoNext = nullptr;
   vk::PhysicalDevicePortabilitySubsetFeaturesKHR portabilityFeatures;
   if (std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) != deviceExtensions.end())
   {
      vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
      physicalDeviceFeatures2.setPNext(&portabilityFeatures);

      physicalDevice.getFeatures2(&physicalDeviceFeatures2);
      deviceCreateInfoNext = &portabilityFeatures;
   }

   vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
      .setQueueCreateInfos(deviceQueueCreateInfos)
      .setPEnabledLayerNames(layers)
      .setPEnabledExtensionNames(deviceExtensions)
      .setPEnabledFeatures(&deviceFeatures)
      .setPNext(deviceCreateInfoNext);

   device = physicalDevice.createDevice(deviceCreateInfo);
   dispatchLoaderDynamic.init(device);

   graphicsQueue = device.getQueue(queueFamilyIndices.graphicsFamily, 0);
   presentQueue = device.getQueue(queueFamilyIndices.presentFamily, 0);

   vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
      .setQueueFamilyIndex(queueFamilyIndices.graphicsFamily)
      .setFlags(vk::CommandPoolCreateFlagBits::eTransient);

   transientCommandPool = device.createCommandPool(commandPoolCreateInfo);

   layoutCache = std::make_unique<DescriptorSetLayoutCache>(*this);
   delayedObjectDestroyer = std::make_unique<DelayedObjectDestroyer>(*this);
}

GraphicsContext::~GraphicsContext()
{
   delayedObjectDestroyer = nullptr;
   layoutCache = nullptr;

   device.destroyCommandPool(transientCommandPool);

   device.destroy();

   instance.destroySurfaceKHR(surface);

#if FORGE_DEBUG
   if (pfnDestroyDebugUtilsMessengerEXT)
   {
      pfnDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
      debugMessenger = nullptr;
   }
#endif // FORGE_DEBUG

   instance.destroy();
}

void GraphicsContext::setSwapchainIndex(uint32_t index)
{
   ASSERT(swapchain && index < swapchain->getImageCount());
   swapchainIndex = index;
}

void GraphicsContext::setFrameIndex(uint32_t index)
{
   ASSERT(index < kMaxFramesInFlight);
   frameIndex = index;

   ASSERT(delayedObjectDestroyer);
   delayedObjectDestroyer->onFrameIndexUpdate();
}

void GraphicsContext::delayedDestroy(uint64_t handle, vk::ObjectType type) const
{
   ASSERT(delayedObjectDestroyer);
   delayedObjectDestroyer->delayedDestroy(handle, type);
}

void GraphicsContext::delayedFree(uint64_t pool, uint64_t handle, vk::ObjectType type) const
{
   ASSERT(delayedObjectDestroyer);
   delayedObjectDestroyer->delayedFree(pool, handle, type);
}
