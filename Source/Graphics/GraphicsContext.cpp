#include "Graphics/GraphicsContext.h"

#include "Core/Log.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/DelayedObjectDestroyer.h"
#include "Graphics/Swapchain.h"

#include "Platform/Window.h"

#include <PlatformUtils/IOUtils.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

      std::vector<const char*> optionalExtensions = { VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME };
#if FORGE_WITH_DEBUG_UTILS
      optionalExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // FORGE_WITH_DEBUG_UTILS

      for (const char* optionalExtension : optionalExtensions)
      {
         if (hasExtensionProperty(extensionProperties, optionalExtension))
         {
            extensions.push_back(optionalExtension);
         }
      }

      return extensions;
   }

   std::vector<const char*> getLayers()
   {
      std::vector<const char*> layers;

#if FORGE_WITH_VALIDATION_LAYERS
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
#endif // FORGE_WITH_VALIDATION_LAYERS

      return layers;
   }

#if FORGE_WITH_VALIDATION_LAYERS
   bool isDebugMessageIgnored(int32_t messageId)
   {
      // VUID-VkGraphicsPipelineCreateInfo-pMultisampleState-09026
      // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/4235
      return messageId == 0x92d66fc1;
   }

   const char* getDebugUtilsMessageTypeName(vk::DebugUtilsMessageTypeFlagBitsEXT messageType)
   {
      switch (messageType)
      {
      case vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral:
         return "general";
      case vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation:
         return "validation";
      case vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance:
         return "performance";
      case vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding:
         return "device address binding";
      default:
         return "unknown";
      }
   }

   VKAPI_ATTR vk::Bool32 VKAPI_CALL vulkanDebugMessageCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
   {
      if (pCallbackData && isDebugMessageIgnored(pCallbackData->messageIdNumber))
      {
         return VK_FALSE;
      }

      std::stringstream ss;
      ss << "Vulkan debug message (type = ";

      bool typeSet = false;
      static const auto appendTypeName = [](std::stringstream& ss, bool& typeSet, vk::DebugUtilsMessageTypeFlagsEXT messageType, vk::DebugUtilsMessageTypeFlagBitsEXT typeBit)
      {
         if (messageType & typeBit)
         {
            if (typeSet)
            {
               ss << ", ";
            }

            ss << getDebugUtilsMessageTypeName(typeBit);
            typeSet = true;
         }
      };

      appendTypeName(ss, typeSet, messageType, vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral);
      appendTypeName(ss, typeSet, messageType, vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
      appendTypeName(ss, typeSet, messageType, vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
      appendTypeName(ss, typeSet, messageType, vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding);

      ss << "): ";

      const char* messageText = pCallbackData ? pCallbackData->pMessage : "none";
      ss << messageText;

      std::string message = ss.str();
      switch (messageSeverity)
      {
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
         LOG_DEBUG(message);
         break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
         LOG_INFO(message);
         break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
         LOG_WARNING(message);
         break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
         ASSERT(false, "%s", message.c_str());
         break;
      default:
         ASSERT(false);
         break;
      }

      return VK_FALSE;
   }

   vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo()
   {
      return vk::DebugUtilsMessengerCreateInfoEXT()
         .setPfnUserCallback(vulkanDebugMessageCallback)
         .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
         .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
   }
#endif // FORGE_WITH_VALIDATION_LAYERS

   std::vector<const char*> getDeviceExtensions(vk::PhysicalDevice physicalDevice)
   {
      static const std::array<const char*, 4> kRequiredDeviceExtensions =
      {
         VK_KHR_SWAPCHAIN_EXTENSION_NAME,
         VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
         VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
         VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
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

   bool getQueueFamilyIndices(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, uint32_t& graphicsFamilyIndex, uint32_t& presentFamilyIndex)
   {
      bool determinedGraphicsFamilyIndex = false;
      bool determinedPresentFamilyIndex = false;

      uint32_t index = 0;
      for (const vk::QueueFamilyProperties& queueFamilyProperties : physicalDevice.getQueueFamilyProperties())
      {
         if (!determinedGraphicsFamilyIndex && (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics))
         {
            determinedGraphicsFamilyIndex = true;
            graphicsFamilyIndex = index;
         }

         if (!determinedPresentFamilyIndex && physicalDevice.getSurfaceSupportKHR(index, surface))
         {
            determinedPresentFamilyIndex = true;
            presentFamilyIndex = index;
         }

         if (determinedGraphicsFamilyIndex && determinedPresentFamilyIndex)
         {
            break;
         }

         ++index;
      }

      return determinedGraphicsFamilyIndex && determinedPresentFamilyIndex;
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

      uint32_t graphicsFamilyIndex = 0;
      uint32_t presentFamilyIndex = 0;
      if (!getQueueFamilyIndices(physicalDevice, surface, graphicsFamilyIndex, presentFamilyIndex))
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

   std::optional<std::filesystem::path> getPipelineCachePath()
   {
      return IOUtils::getAbsoluteAppDataPath(FORGE_PROJECT_NAME, "PipelineCache.bin");
   }
}

#if FORGE_WITH_GPU_MEMORY_TRACKING
class GraphicsContext::MemoryTracker
{
public:
   static void onAllocate(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData)
   {
      MemoryTracker* self = reinterpret_cast<MemoryTracker*>(pUserData);
      ASSERT(self);

      self->onVmaAllocate(allocator, memoryType, size);
   }

   static void onFree(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData)
   {
      MemoryTracker* self = reinterpret_cast<MemoryTracker*>(pUserData);
      ASSERT(self);

      self->onVmaFree(allocator, memoryType, size);
   }

   ~MemoryTracker()
   {
      for (const auto& [memoryType, allocatedBytes] : memoryUsageByType)
      {
         ASSERT(allocatedBytes == 0, "%llu bytes leaked for memory type %u", allocatedBytes, memoryType);
      }
   }

   void setAllocator(VmaAllocator allocator)
   {
      vmaAllocator = allocator;
   }

private:
   void onVmaAllocate(VmaAllocator allocator, uint32_t memoryType, VkDeviceSize size)
   {
      ASSERT(allocator == vmaAllocator);

      auto location = memoryUsageByType.find(memoryType);
      if (location == memoryUsageByType.end())
      {
         location = memoryUsageByType.emplace(memoryType, 0).first;
      }
      location->second += size;
   }

   void onVmaFree(VmaAllocator allocator, uint32_t memoryType, VkDeviceSize size)
   {
      ASSERT(allocator == vmaAllocator);

      ASSERT(memoryUsageByType.contains(memoryType));
      ASSERT(memoryUsageByType[memoryType] >= size);
      memoryUsageByType[memoryType] -= size;
   }

   VmaAllocator vmaAllocator = nullptr;
   std::unordered_map<uint32_t, VkDeviceSize> memoryUsageByType;
};
#endif // FORGE_WITH_GPU_MEMORY_TRACKING

// static
const vk::detail::DispatchLoaderDynamic& GraphicsContext::GetDynamicLoader()
{
   return dispatchLoaderDynamic;
}

// static
vk::detail::DispatchLoaderDynamic GraphicsContext::dispatchLoaderDynamic;

GraphicsContext::GraphicsContext(Window& window)
{
   vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
      .setPApplicationName(FORGE_PROJECT_NAME)
      .setApplicationVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setPEngineName(FORGE_PROJECT_NAME)
      .setEngineVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setApiVersion(kVulkanTargetVersion);

   std::vector<const char*> extensions = getExtensions(window);
   std::vector<const char*> layers = getLayers();

   vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
      .setPApplicationInfo(&applicationInfo)
      .setPEnabledExtensionNames(extensions)
      .setPEnabledLayerNames(layers);

   if (std::find(extensions.begin(), extensions.end(), VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) != extensions.end())
   {
      createInfo.setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
   }

#if FORGE_WITH_VALIDATION_LAYERS
   VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = static_cast<VkDebugUtilsMessengerCreateInfoEXT>(createDebugMessengerCreateInfo());
   createInfo.setPNext(&debugUtilsMessengerCreateInfo);
#endif // FORGE_WITH_VALIDATION_LAYERS

   instance = vk::createInstance(createInfo);
   dispatchLoaderDynamic.init(instance, vk::Device());

#if FORGE_WITH_VALIDATION_LAYERS
#  define FIND_VULKAN_FUNCTION(instance, name) reinterpret_cast<PFN_##name>(window.getInstanceProcAddress(instance, #name))
   PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = FIND_VULKAN_FUNCTION(instance, vkCreateDebugUtilsMessengerEXT);
   pfnDestroyDebugUtilsMessengerEXT = FIND_VULKAN_FUNCTION(instance, vkDestroyDebugUtilsMessengerEXT);
#  undef FIND_VULKAN_FUNCTION

   if (pfnCreateDebugUtilsMessengerEXT)
   {
      if (pfnCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
      {
         ASSERT(false);
      }
   }
#endif // FORGE_WITH_VALIDATION_LAYERS

   surface = window.createSurface(instance);

   physicalDevice = selectBestPhysicalDevice(instance.enumeratePhysicalDevices(), surface);
   if (!physicalDevice)
   {
      throw std::runtime_error("Failed to find a suitable GPU");
   }
   physicalDeviceProperties = physicalDevice.getProperties();
   physicalDeviceFeatures = physicalDevice.getFeatures();

   if (!getQueueFamilyIndices(physicalDevice, surface, graphicsFamilyIndex, presentFamilyIndex))
   {
      throw std::runtime_error("Failed to get queue family indices");
   }

   float queuePriority = 1.0f;
   std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
   for (uint32_t queueFamilyIndex : std::set<uint32_t>{ graphicsFamilyIndex, presentFamilyIndex })
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

   void* portabilityFeaturesPointer = nullptr;
   vk::PhysicalDevicePortabilitySubsetFeaturesKHR portabilityFeatures;
   if (std::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) != deviceExtensions.end())
   {
      vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
      physicalDeviceFeatures2.setPNext(&portabilityFeatures);

      physicalDevice.getFeatures2(&physicalDeviceFeatures2);
      portabilityFeaturesPointer = &portabilityFeatures;
   }

   vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures()
      .setDynamicRendering(true)
      .setPNext(portabilityFeaturesPointer);

   vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
      .setQueueCreateInfos(deviceQueueCreateInfos)
      .setPEnabledLayerNames(layers)
      .setPEnabledExtensionNames(deviceExtensions)
      .setPEnabledFeatures(&deviceFeatures)
      .setPNext(&dynamicRenderingFeatures);

   device = physicalDevice.createDevice(deviceCreateInfo);
   dispatchLoaderDynamic.init(device);

   graphicsQueue = device.getQueue(graphicsFamilyIndex, 0);
   presentQueue = device.getQueue(presentFamilyIndex, 0);

   vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
      .setQueueFamilyIndex(graphicsFamilyIndex)
      .setFlags(vk::CommandPoolCreateFlagBits::eTransient);

   transientCommandPool = device.createCommandPool(commandPoolCreateInfo);

   std::optional<std::vector<uint8_t>> pipelineCacheData;
   if (std::optional<std::filesystem::path> pipelineCachePath = getPipelineCachePath())
   {
      pipelineCacheData = IOUtils::readBinaryFile(*pipelineCachePath);
   }

   vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
   if (pipelineCacheData)
   {
      pipelineCacheCreateInfo.setInitialData<uint8_t>(*pipelineCacheData);
   }

   pipelineCache = device.createPipelineCache(pipelineCacheCreateInfo);

   VmaAllocatorCreateInfo allocatorCreateInfo{};
   allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT; // Everything is currently on one thread
   allocatorCreateInfo.physicalDevice = physicalDevice;
   allocatorCreateInfo.device = device;
   allocatorCreateInfo.instance = instance;
   allocatorCreateInfo.vulkanApiVersion = kVulkanTargetVersion;

#if FORGE_WITH_GPU_MEMORY_TRACKING
   memoryTracker = std::make_unique<MemoryTracker>();

   VmaDeviceMemoryCallbacks memoryCallbacks{};
   memoryCallbacks.pfnAllocate = &GraphicsContext::MemoryTracker::onAllocate;
   memoryCallbacks.pfnFree = &GraphicsContext::MemoryTracker::onFree;
   memoryCallbacks.pUserData = memoryTracker.get();

   allocatorCreateInfo.pDeviceMemoryCallbacks = &memoryCallbacks;
#endif // FORGE_WITH_GPU_MEMORY_TRACKING

   if (vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator) != VK_SUCCESS)
   {
      throw std::runtime_error("Failed to create VMA allocator");
   }

#if FORGE_WITH_GPU_MEMORY_TRACKING
   memoryTracker->setAllocator(vmaAllocator);
#endif // FORGE_WITH_GPU_MEMORY_TRACKING

   delayedObjectDestroyer = std::make_unique<DelayedObjectDestroyer>(*this);
   layoutCache = std::make_unique<DescriptorSetLayoutCache>(*this);
}

GraphicsContext::~GraphicsContext()
{
   layoutCache = nullptr;
   delayedObjectDestroyer = nullptr;

   vmaDestroyAllocator(vmaAllocator);
   vmaAllocator = nullptr;

#if FORGE_WITH_GPU_MEMORY_TRACKING
   memoryTracker = nullptr;
#endif // FORGE_WITH_GPU_MEMORY_TRACKING

   if (std::optional<std::filesystem::path> pipelineCachePath = getPipelineCachePath())
   {
      std::vector<uint8_t> pipelineCacheData = device.getPipelineCacheData(pipelineCache);
      IOUtils::writeBinaryFile(*pipelineCachePath, pipelineCacheData);
   }

   device.destroyPipelineCache(pipelineCache);
   device.destroyCommandPool(transientCommandPool);

   device.destroy();

   instance.destroySurfaceKHR(surface);

#if FORGE_WITH_VALIDATION_LAYERS
   if (pfnDestroyDebugUtilsMessengerEXT)
   {
      pfnDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
      debugMessenger = nullptr;
   }
#endif // FORGE_WITH_VALIDATION_LAYERS

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

vk::DescriptorSetLayout GraphicsContext::getDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& createInfo) const
{
   ASSERT(layoutCache);
   return layoutCache->getLayout(createInfo);
}

void GraphicsContext::delayedDestroy(uint64_t handle, vk::ObjectType type, VmaAllocation allocation) const
{
   ASSERT(delayedObjectDestroyer);
   delayedObjectDestroyer->delayedDestroy(handle, type, allocation);
}

void GraphicsContext::delayedFree(uint64_t pool, uint64_t handle, vk::ObjectType type) const
{
   ASSERT(delayedObjectDestroyer);
   delayedObjectDestroyer->delayedFree(pool, handle, type);
}
