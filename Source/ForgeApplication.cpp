#include "ForgeApplication.h"

#include "Core/Log.h"

#include "Graphics/Command.h"
#include "Graphics/Memory.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PlatformUtils/IOUtils.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
   const int kInitialWindowWidth = 1280;
   const int kInitialWindowHeight = 720;
   const std::size_t kMaxFramesInFlight = 2;

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

#define FIND_VULKAN_FUNCTION(instance, name) reinterpret_cast<PFN_##name>(glfwGetInstanceProcAddress(instance, #name))

   VkDebugUtilsMessengerEXT createDebugMessenger(vk::Instance instance)
   {
      VkDebugUtilsMessengerEXT debugMessenger = nullptr;
      if (auto pfnCreateDebugUtilsMessengerEXT = FIND_VULKAN_FUNCTION(instance, vkCreateDebugUtilsMessengerEXT))
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
      if (auto pfnDestroyDebugUtilsMessengerEXT = FIND_VULKAN_FUNCTION(instance, vkDestroyDebugUtilsMessengerEXT))
      {
         pfnDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
         debugMessenger = nullptr;
      }
   }

#undef FIND_VULKAN_FUNCTION

#endif // FORGE_DEBUG

   void framebufferSizeCallback(GLFWwindow* window, int width, int height)
   {
      if (ForgeApplication* forgeApplication = reinterpret_cast<ForgeApplication*>(glfwGetWindowUserPointer(window)))
      {
         forgeApplication->onFramebufferResized();
      }
   }

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

   std::vector<const char*> getExtensions()
   {
      std::vector<const char*> extensions;

      uint32_t glfwRequiredExtensionCount = 0;
      const char** glfwRequiredExtensionNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
      extensions.reserve(glfwRequiredExtensionCount);

      std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();
      for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
      {
         const char* requiredExtension = glfwRequiredExtensionNames[i];

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

   std::vector<const char*> getDeviceExtensions(vk::PhysicalDevice physicalDevice)
   {
      static const std::array<const char*, 1> kRequiredDeviceExtensions =
      {
         VK_KHR_SWAPCHAIN_EXTENSION_NAME
      };

      std::vector<const char*> deviceExtensions;
      deviceExtensions.reserve(kRequiredDeviceExtensions.size());

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

      return deviceExtensions;
   }

   struct SwapChainSupportDetails
   {
      vk::SurfaceCapabilitiesKHR capabilities;
      std::vector<vk::SurfaceFormatKHR> formats;
      std::vector<vk::PresentModeKHR> presentModes;

      bool isValid() const
      {
         return !formats.empty() && !presentModes.empty();
      }
   };

   SwapChainSupportDetails getSwapChainSupportDetails(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
   {
      SwapChainSupportDetails details;

      details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
      details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
      details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

      return details;
   }

   vk::SurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
   {
      for (const vk::SurfaceFormatKHR& format : formats)
      {
         if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
         {
            return format;
         }
      }

      if (!formats.empty())
      {
         return formats[0];
      }

      throw std::runtime_error("No swapchain formats available");
   }

   vk::PresentModeKHR chooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
   {
      static const auto containsPresentMode = [](const std::vector<vk::PresentModeKHR>& presentModes, vk::PresentModeKHR presentMode)
      {
         return std::find(presentModes.begin(), presentModes.end(), presentMode) != presentModes.end();
      };

      if (containsPresentMode(presentModes, vk::PresentModeKHR::eMailbox))
      {
         return vk::PresentModeKHR::eMailbox;
      }

      if (containsPresentMode(presentModes, vk::PresentModeKHR::eFifo))
      {
         return vk::PresentModeKHR::eFifo;
      }

      if (!presentModes.empty())
      {
         return presentModes[0];
      }

      throw std::runtime_error("No swapchain present modes available");
   }

   vk::Extent2D chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
   {
      if (capabilities.currentExtent.width != UINT32_MAX)
      {
         return capabilities.currentExtent;
      }

      int framebufferWidth = 0;
      int framebufferHeight = 0;
      glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
      if (framebufferWidth < 0 || framebufferHeight < 0)
      {
         throw std::runtime_error("Negative framebuffer size");
      }

      vk::Extent2D extent(static_cast<uint32_t>(framebufferWidth), static_cast<uint32_t>(framebufferHeight));
      extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
      extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

      return extent;
   }

   int getPhysicalDeviceScore(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
   {
      try
      {
         getDeviceExtensions(physicalDevice);
      }
      catch (const std::runtime_error&)
      {
         return -1;
      }

      SwapChainSupportDetails swapChainSupportDetails = getSwapChainSupportDetails(physicalDevice, surface);
      if (!swapChainSupportDetails.isValid())
      {
         return -1;
      }

      std::optional<QueueFamilyIndices> queueFamilyIndices = QueueFamilyIndices::get(physicalDevice, surface);
      if (!queueFamilyIndices)
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

   vk::ImageView createImageView(const VulkanContext& context, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
   {
      vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
         .setAspectMask(aspectFlags)
         .setBaseMipLevel(0)
         .setLevelCount(mipLevels)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
         .setImage(image)
         .setViewType(vk::ImageViewType::e2D)
         .setFormat(format)
         .setSubresourceRange(subresourceRange);

      return context.device.createImageView(createInfo);
   }

   vk::Format findSupportedFormat(const VulkanContext& context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
   {
      for (vk::Format format : candidates)
      {
         vk::FormatProperties properties = context.physicalDevice.getFormatProperties(format);

         switch (tiling)
         {
         case vk::ImageTiling::eOptimal:
            if ((properties.optimalTilingFeatures & features) == features)
            {
               return format;
            }
            break;
         case vk::ImageTiling::eLinear:
            if ((properties.linearTilingFeatures & features) == features)
            {
               return format;
            }
            break;
         default:
            break;
         }
      }

      throw std::runtime_error("Failed to find supported format");
   }

   vk::Format findDepthFormat(const VulkanContext& context)
   {
      return findSupportedFormat(context, { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
   }

   vk::SampleCountFlagBits getMaxSampleCount(const VulkanContext& context)
   {
      vk::SampleCountFlags flags = context.physicalDeviceProperties.limits.framebufferColorSampleCounts & context.physicalDeviceProperties.limits.framebufferDepthSampleCounts;

      if (flags & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
      if (flags & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
      if (flags & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
      if (flags & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
      if (flags & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
      if (flags & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

      return vk::SampleCountFlagBits::e1;
   }
}

ForgeApplication::ForgeApplication()
{
   initializeGlfw();
   initializeVulkan();
   initializeTransientCommandPool();
   initializeSwapchain();
   initializeRenderPass();
   initializeDescriptorSetLayouts();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeUniformBuffers();
   initializeMesh();
   initializeDescriptorPool();
   initializeDescriptorSets();
   initializeCommandBuffers();
   initializeSyncObjects();
}

ForgeApplication::~ForgeApplication()
{
   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateDescriptorSets();
   terminateDescriptorPool();
   terminateMesh();
   terminateUniformBuffers();
   terminateFramebuffers();
   terminateGraphicsPipeline();
   terminateDescriptorSetLayouts();
   terminateRenderPass();
   terminateSwapchain();
   terminateTransientCommandPool();
   terminateVulkan();
   terminateGlfw();
}

void ForgeApplication::run()
{
   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();
      render();
   }

   context.device.waitIdle();
}

void ForgeApplication::render()
{
   if (framebufferResized)
   {
      recreateSwapchain();
   }

   vk::Result frameWaitResult = context.device.waitForFences({ frameFences[frameIndex] }, true, UINT64_MAX);
   if (frameWaitResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to wait for frame fence");
   }

   vk::ResultValue<uint32_t> imageIndexResultValue = context.device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   if (imageIndexResultValue.result == vk::Result::eErrorOutOfDateKHR)
   {
      recreateSwapchain();
      imageIndexResultValue = context.device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   }
   if (imageIndexResultValue.result != vk::Result::eSuccess && imageIndexResultValue.result != vk::Result::eSuboptimalKHR)
   {
      throw std::runtime_error("Failed to acquire swapchain image");
   }
   uint32_t imageIndex = imageIndexResultValue.value;

   if (imageFences[imageIndex])
   {
      // If a previous frame is still using the image, wait for it to complete
      vk::Result imageWaitResult = context.device.waitForFences({ imageFences[imageIndex] }, true, UINT64_MAX);
      if (imageWaitResult != vk::Result::eSuccess)
      {
         throw std::runtime_error("Failed to wait for image fence");
      }
   }
   imageFences[imageIndex] = frameFences[frameIndex];

   std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[frameIndex] };
   std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
   static_assert(waitSemaphores.size() == waitStages.size(), "Wait semaphores and wait stages must be parallel arrays");

   updateUniformBuffers(imageIndex);

   std::array<vk::Semaphore, 1> signalSemaphores = { renderFinishedSemaphores[frameIndex] };
   vk::SubmitInfo submitInfo = vk::SubmitInfo()
      .setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphores.size()))
      .setPWaitSemaphores(waitSemaphores.data())
      .setPWaitDstStageMask(waitStages.data())
      .setCommandBufferCount(1)
      .setPCommandBuffers(&commandBuffers[imageIndex])
      .setSignalSemaphoreCount(static_cast<uint32_t>(signalSemaphores.size()))
      .setPSignalSemaphores(signalSemaphores.data());

   context.device.resetFences({ frameFences[frameIndex] });
   context.graphicsQueue.submit({ submitInfo }, frameFences[frameIndex]);

   std::array<vk::SwapchainKHR, 1> swapchains = { swapchain };

   vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
      .setWaitSemaphoreCount(static_cast<uint32_t>(signalSemaphores.size()))
      .setPWaitSemaphores(signalSemaphores.data())
      .setSwapchainCount(static_cast<uint32_t>(swapchains.size()))
      .setPSwapchains(swapchains.data())
      .setPImageIndices(&imageIndex);

   vk::Result presentResult = context.presentQueue.presentKHR(presentInfo);
   if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
   {
      recreateSwapchain();
   }
   else if (presentResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to present swapchain image");
   }

   frameIndex = (frameIndex + 1) % kMaxFramesInFlight;
}

void ForgeApplication::updateUniformBuffers(uint32_t index)
{
   double time = glfwGetTime();

   glm::mat4 worldToView = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   glm::mat4 viewToClip = glm::perspective(glm::radians(70.0f), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, 0.1f, 10.0f);
   viewToClip[1][1] *= -1.0f;

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;

   MeshUniformData meshUniformData;
   meshUniformData.localToWorld = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * static_cast<float>(time), glm::vec3(0.0f, 0.0f, 1.0f));

   viewUniformBuffer->update(context, viewUniformData, index);
   meshUniformBuffer->update(context, meshUniformData, index);
}

void ForgeApplication::recreateSwapchain()
{
   int width = 0;
   int height = 0;
   glfwGetFramebufferSize(window, &width, &height);
   while (width == 0 || height == 0)
   {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
   }

   context.device.waitIdle();

   terminateCommandBuffers(true);
   terminateDescriptorSets();
   terminateDescriptorPool();
   terminateUniformBuffers();
   terminateFramebuffers();
   terminateGraphicsPipeline();
   terminateRenderPass();
   terminateSwapchain();

   initializeSwapchain();
   initializeRenderPass();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeUniformBuffers();
   initializeDescriptorPool();
   initializeDescriptorSets();
   initializeCommandBuffers();

   framebufferResized = false;
}

void ForgeApplication::initializeGlfw()
{
   ASSERT(!window);

   if (!glfwInit())
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   if (!glfwVulkanSupported())
   {
      throw std::runtime_error("Vulkan is not supported on this machine");
   }

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   window = glfwCreateWindow(kInitialWindowWidth, kInitialWindowHeight, FORGE_PROJECT_NAME, nullptr, nullptr);

   glfwSetWindowUserPointer(window, this);
   glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

void ForgeApplication::terminateGlfw()
{
   ASSERT(window);

   glfwDestroyWindow(window);
   window = nullptr;

   glfwTerminate();
}

void ForgeApplication::initializeVulkan()
{
   vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
      .setPApplicationName(FORGE_PROJECT_NAME)
      .setApplicationVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setPEngineName(FORGE_PROJECT_NAME)
      .setEngineVersion(VK_MAKE_VERSION(FORGE_VERSION_MAJOR, FORGE_VERSION_MINOR, FORGE_VERSION_PATCH))
      .setApiVersion(VK_API_VERSION_1_1);

   std::vector<const char*> extensions = getExtensions();
   std::vector<const char*> layers = getLayers();

   vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
      .setPApplicationInfo(&applicationInfo)
      .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
      .setPpEnabledExtensionNames(extensions.data())
      .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
      .setPpEnabledLayerNames(layers.data());

#if FORGE_DEBUG
   VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = createDebugMessengerCreateInfo();
   createInfo = createInfo.setPNext(&debugUtilsMessengerCreateInfo);
#endif // FORGE_DEBUG

   context.instance = vk::createInstance(createInfo);

#if FORGE_DEBUG
   debugMessenger = createDebugMessenger(context.instance);
#endif // FORGE_DEBUG

   VkSurfaceKHR vkSurface = nullptr;
   if (glfwCreateWindowSurface(context.instance, window, nullptr, &vkSurface) != VK_SUCCESS)
   {
      throw std::runtime_error("Failed to create window surface");
   }
   context.surface = vkSurface;

   context.physicalDevice = selectBestPhysicalDevice(context.instance.enumeratePhysicalDevices(), context.surface);
   if (!context.physicalDevice)
   {
      throw std::runtime_error("Failed to find a suitable GPU");
   }
   context.physicalDeviceProperties = context.physicalDevice.getProperties();
   context.physicalDeviceFeatures = context.physicalDevice.getFeatures();
   msaaSamples = getMaxSampleCount(context);

   std::optional<QueueFamilyIndices> queueFamilyIndices = QueueFamilyIndices::get(context.physicalDevice, context.surface);
   if (!queueFamilyIndices)
   {
      throw std::runtime_error("Failed to get queue family indices");
   }
   context.queueFamilyIndices = *queueFamilyIndices;
   std::set<uint32_t> uniqueQueueIndices = context.queueFamilyIndices.getUniqueIndices();

   float queuePriority = 1.0f;
   std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
   for (uint32_t queueFamilyIndex : uniqueQueueIndices)
   {
      deviceQueueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
         .setQueueFamilyIndex(queueFamilyIndex)
         .setQueueCount(1)
         .setPQueuePriorities(&queuePriority));
   }

   std::vector<const char*> deviceExtensions = getDeviceExtensions(context.physicalDevice);

   vk::PhysicalDeviceFeatures deviceFeatures;
   deviceFeatures.setSamplerAnisotropy(context.physicalDeviceFeatures.samplerAnisotropy);
   deviceFeatures.setSampleRateShading(true);

   vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
      .setPQueueCreateInfos(deviceQueueCreateInfos.data())
      .setQueueCreateInfoCount(static_cast<uint32_t>(deviceQueueCreateInfos.size()))
      .setPpEnabledExtensionNames(deviceExtensions.data())
      .setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()))
      .setPEnabledFeatures(&deviceFeatures);

#if FORGE_DEBUG
   deviceCreateInfo = deviceCreateInfo
      .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
      .setPpEnabledLayerNames(layers.data());
#endif // FORGE_DEBUG

   context.device = context.physicalDevice.createDevice(deviceCreateInfo);

   context.graphicsQueue = context.device.getQueue(context.queueFamilyIndices.graphicsFamily, 0);
   context.presentQueue = context.device.getQueue(context.queueFamilyIndices.presentFamily, 0);
}

void ForgeApplication::terminateVulkan()
{
   meshResourceManager.clear();
   textureResourceManager.clear();

   context.device.destroy();
   context.device = nullptr;

   context.instance.destroySurfaceKHR(context.surface);
   context.surface = nullptr;

#if FORGE_DEBUG
   destroyDebugMessenger(context.instance, debugMessenger);
#endif // FORGE_DEBUG

   context.instance.destroy();
   context.instance = nullptr;
}

void ForgeApplication::initializeSwapchain()
{
   SwapChainSupportDetails swapChainSupportDetails = getSwapChainSupportDetails(context.physicalDevice, context.surface);
   ASSERT(swapChainSupportDetails.isValid());

   vk::SurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(swapChainSupportDetails.formats);
   vk::PresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainSupportDetails.presentModes);
   vk::Extent2D extent = chooseSwapChainExtent(swapChainSupportDetails.capabilities, window);

   uint32_t desiredMinImageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
   uint32_t minImageCount = swapChainSupportDetails.capabilities.maxImageCount > 0 ? std::min(swapChainSupportDetails.capabilities.maxImageCount, desiredMinImageCount) : desiredMinImageCount;

   vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
      .setSurface(context.surface)
      .setMinImageCount(minImageCount)
      .setImageFormat(surfaceFormat.format)
      .setImageColorSpace(surfaceFormat.colorSpace)
      .setImageExtent(extent)
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPreTransform(swapChainSupportDetails.capabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(presentMode)
      .setClipped(true);

   if (context.queueFamilyIndices.graphicsFamily == context.queueFamilyIndices.presentFamily)
   {
      swapchainCreateInfo = swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
   }
   else
   {
      std::array<uint32_t, 2> indices = { context.queueFamilyIndices.graphicsFamily, context.queueFamilyIndices.presentFamily };

      swapchainCreateInfo = swapchainCreateInfo
         .setImageSharingMode(vk::SharingMode::eConcurrent)
         .setQueueFamilyIndices(indices);
   }

   swapchain = context.device.createSwapchainKHR(swapchainCreateInfo);
   swapchainImages = context.device.getSwapchainImagesKHR(swapchain);
   swapchainImageFormat = surfaceFormat.format;
   swapchainExtent = extent;

   swapchainImageViews.reserve(swapchainImages.size());
   for (vk::Image swapchainImage : swapchainImages)
   {
      swapchainImageViews.push_back(createImageView(context, swapchainImage, swapchainImageFormat, vk::ImageAspectFlagBits::eColor, 1));
   }

   {
      ImageProperties colorImageProperties;
      colorImageProperties.format = swapchainImageFormat;
      colorImageProperties.width = swapchainExtent.width;
      colorImageProperties.height = swapchainExtent.height;

      TextureProperties colorTextureProperties;
      colorTextureProperties.sampleCount = msaaSamples;
      colorTextureProperties.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
      colorTextureProperties.aspects = vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout colorInitialLayout;
      colorInitialLayout.layout = vk::ImageLayout::eColorAttachmentOptimal;
      colorInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

      colorTexture = Texture(context, colorImageProperties, colorTextureProperties, colorInitialLayout);
   }

   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = findDepthFormat(context);
      depthImageProperties.width = swapchainExtent.width;
      depthImageProperties.height = swapchainExtent.height;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = msaaSamples;
      depthTextureProperties.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
      depthTextureProperties.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

      TextureInitialLayout depthInitialLayout;
      depthInitialLayout.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);

      depthTexture = Texture(context, depthImageProperties, depthTextureProperties, depthInitialLayout);
   }
}

void ForgeApplication::terminateSwapchain()
{
   depthTexture.reset();
   colorTexture.reset();

   for (vk::ImageView swapchainImageView : swapchainImageViews)
   {
      context.device.destroyImageView(swapchainImageView);
   }
   swapchainImageViews.clear();

   context.device.destroySwapchainKHR(swapchain);
   swapchain = nullptr;
}

void ForgeApplication::initializeRenderPass()
{
   vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
      .setFormat(colorTexture->getImageProperties().format)
      .setSamples(colorTexture->getTextureProperties().sampleCount)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

   vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
      .setAttachment(0)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

   std::array<vk::AttachmentReference, 1> colorAttachments = { colorAttachmentReference };

   vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
      .setFormat(depthTexture->getImageProperties().format)
      .setSamples(depthTexture->getTextureProperties().sampleCount)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

   vk::AttachmentReference depthAttachmentReference = vk::AttachmentReference()
      .setAttachment(1)
      .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

   vk::AttachmentDescription colorAttachmentResolve = vk::AttachmentDescription()
      .setFormat(swapchainImageFormat)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

   vk::AttachmentReference colorAttachmentResolveReference = vk::AttachmentReference()
      .setAttachment(2)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

   std::array<vk::AttachmentReference, 1> resolveAttachments = { colorAttachmentResolveReference };

   vk::SubpassDescription subpassDescription = vk::SubpassDescription()
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachments(colorAttachments)
      .setPDepthStencilAttachment(&depthAttachmentReference)
      .setResolveAttachments(resolveAttachments);

   vk::SubpassDependency subpassDependency = vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask({})
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

   std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
   vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
      .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
      .setPAttachments(attachments.data())
      .setSubpassCount(1)
      .setPSubpasses(&subpassDescription)
      .setDependencyCount(1)
      .setPDependencies(&subpassDependency);

   renderPass = context.device.createRenderPass(renderPassCreateInfo);
}

void ForgeApplication::terminateRenderPass()
{
   context.device.destroyRenderPass(renderPass);
   renderPass = nullptr;
}

void ForgeApplication::initializeDescriptorSetLayouts()
{
   vk::DescriptorSetLayoutBinding viewUniformBufferLayoutBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);

   vk::DescriptorSetLayoutBinding meshUniformBufferLayoutBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);

   vk::DescriptorSetLayoutBinding samplerLayoutBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(1)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eFragment);

   vk::DescriptorSetLayoutCreateInfo frameLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
      .setBindingCount(1)
      .setPBindings(&viewUniformBufferLayoutBinding);
   frameDescriptorSetLayout = context.device.createDescriptorSetLayout(frameLayoutCreateInfo);

   std::array<vk::DescriptorSetLayoutBinding, 2> drawLayoutBindings =
   {
      meshUniformBufferLayoutBinding,
      samplerLayoutBinding
   };
   vk::DescriptorSetLayoutCreateInfo drawLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
      .setBindingCount(static_cast<uint32_t>(drawLayoutBindings.size()))
      .setPBindings(drawLayoutBindings.data());
   drawDescriptorSetLayout = context.device.createDescriptorSetLayout(drawLayoutCreateInfo);
}

void ForgeApplication::terminateDescriptorSetLayouts()
{
   context.device.destroyDescriptorSetLayout(frameDescriptorSetLayout);
   frameDescriptorSetLayout = nullptr;

   context.device.destroyDescriptorSetLayout(drawDescriptorSetLayout);
   drawDescriptorSetLayout = nullptr;
}

void ForgeApplication::initializeGraphicsPipeline()
{
   ShaderModuleHandle vertModuleHandle = shaderModuleResourceManager.load("Resources/Shaders/Triangle.vert.spv", context);
   ShaderModuleHandle fragModuleHandle = shaderModuleResourceManager.load("Resources/Shaders/Triangle.frag.spv", context);

   const ShaderModule* vertShaderModule = shaderModuleResourceManager.get(vertModuleHandle);
   const ShaderModule* fragShaderModule = shaderModuleResourceManager.get(fragModuleHandle);
   if (!vertShaderModule || !fragShaderModule)
   {
      throw std::runtime_error(std::string("Failed to load shader"));
   }

   vk::PipelineShaderStageCreateInfo vertShaderStageCreateinfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vertShaderModule->getShaderModule())
      .setPName("main");

   vk::SpecializationMapEntry specializationMapEntry = vk::SpecializationMapEntry()
      .setConstantID(0)
      .setOffset(0)
      .setSize(sizeof(VkBool32));

   std::array<vk::SpecializationMapEntry, 1> specializationMapEntries = { specializationMapEntry };
   std::array<VkBool32, 1> specializationData = { true };

   vk::SpecializationInfo specializationInfo = vk::SpecializationInfo()
      .setMapEntries(specializationMapEntries)
      .setData<VkBool32>(specializationData);

   vk::PipelineShaderStageCreateInfo fragShaderStageCreateinfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&specializationInfo);

   std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageCreateinfo, fragShaderStageCreateinfo };

   vk::VertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
   std::array<vk::VertexInputAttributeDescription, 3> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
   vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo()
      .setVertexBindingDescriptionCount(1)
      .setPVertexBindingDescriptions(&vertexBindingDescription)
      .setVertexAttributeDescriptionCount(static_cast<uint32_t>(vertexAttributeDescriptions.size()))
      .setPVertexAttributeDescriptions(vertexAttributeDescriptions.data());

   vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
      .setTopology(vk::PrimitiveTopology::eTriangleList);

   vk::Viewport viewport = vk::Viewport()
      .setX(0.0f)
      .setY(0.0f)
      .setWidth(static_cast<float>(swapchainExtent.width))
      .setHeight(static_cast<float>(swapchainExtent.height))
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);

   vk::Rect2D scissor = vk::Rect2D()
      .setExtent(swapchainExtent);

   vk::PipelineViewportStateCreateInfo viewportStateCreateInfo = vk::PipelineViewportStateCreateInfo()
      .setViewportCount(1)
      .setPViewports(&viewport)
      .setScissorCount(1)
      .setPScissors(&scissor);

   vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo()
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0f) // TODO necessary?
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eCounterClockwise);

   vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
      .setRasterizationSamples(msaaSamples)
      .setSampleShadingEnable(true)
      .setMinSampleShading(0.2f);

   vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
      .setDepthTestEnable(true)
      .setDepthWriteEnable(true)
      .setDepthCompareOp(vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(false)
      .setStencilTestEnable(false);

   vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
      .setLogicOpEnable(false)
      .setAttachmentCount(1)
      .setPAttachments(&colorBlendAttachmentState);

   std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts = { frameDescriptorSetLayout, drawDescriptorSetLayout };
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts);
   pipelineLayout = context.device.createPipelineLayout(pipelineLayoutCreateInfo);

   vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
      .setStages(shaderStages)
      .setPVertexInputState(&vertexInputCreateInfo)
      .setPInputAssemblyState(&inputAssemblyCreateInfo)
      .setPViewportState(&viewportStateCreateInfo)
      .setPRasterizationState(&rasterizationStateCreateInfo)
      .setPMultisampleState(&multisampleStateCreateInfo)
      .setPDepthStencilState(&depthStencilCreateInfo)
      .setPColorBlendState(&colorBlendStateCreateInfo)
      .setPDynamicState(nullptr)
      .setLayout(pipelineLayout)
      .setRenderPass(renderPass)
      .setSubpass(0)
      .setBasePipelineHandle(nullptr)
      .setBasePipelineIndex(-1);

   graphicsPipeline = context.device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo).value;

   shaderModuleResourceManager.unload(vertModuleHandle);
   shaderModuleResourceManager.unload(fragModuleHandle);
}

void ForgeApplication::terminateGraphicsPipeline()
{
   shaderModuleResourceManager.clear();

   context.device.destroyPipeline(graphicsPipeline);
   graphicsPipeline = nullptr;

   context.device.destroyPipelineLayout(pipelineLayout);
   pipelineLayout = nullptr;
}

void ForgeApplication::initializeFramebuffers()
{
   ASSERT(swapchainFramebuffers.empty());
   swapchainFramebuffers.reserve(swapchainImageViews.size());

   for (vk::ImageView swapchainImageView : swapchainImageViews)
   {
      std::array<vk::ImageView, 3> attachments = { colorTexture->getDefaultView(), depthTexture->getDefaultView(), swapchainImageView };

      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
         .setPAttachments(attachments.data())
         .setWidth(swapchainExtent.width)
         .setHeight(swapchainExtent.height)
         .setLayers(1);

      swapchainFramebuffers.push_back(context.device.createFramebuffer(framebufferCreateInfo));
   }
}

void ForgeApplication::terminateFramebuffers()
{
   for (vk::Framebuffer swapchainFramebuffer : swapchainFramebuffers)
   {
      context.device.destroyFramebuffer(swapchainFramebuffer);
   }
   swapchainFramebuffers.clear();
}

void ForgeApplication::initializeTransientCommandPool()
{
   ASSERT(!context.transientCommandPool);

   vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
      .setQueueFamilyIndex(context.queueFamilyIndices.graphicsFamily)
      .setFlags(vk::CommandPoolCreateFlagBits::eTransient);

   context.transientCommandPool = context.device.createCommandPool(commandPoolCreateInfo);
}

void ForgeApplication::terminateTransientCommandPool()
{
   context.device.destroyCommandPool(context.transientCommandPool);
   context.transientCommandPool = nullptr;
}

void ForgeApplication::initializeUniformBuffers()
{
   uint32_t swapchainImageCount = static_cast<uint32_t>(swapchainImages.size());

   viewUniformBuffer = UniformBuffer<ViewUniformData>(context, swapchainImageCount);
   meshUniformBuffer = UniformBuffer<MeshUniformData>(context, swapchainImageCount);
}

void ForgeApplication::terminateUniformBuffers()
{
   meshUniformBuffer.reset();
   viewUniformBuffer.reset();
}

void ForgeApplication::initializeDescriptorPool()
{
   vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(static_cast<uint32_t>(swapchainImages.size() * 2));

   vk::DescriptorPoolSize samplerPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(static_cast<uint32_t>(swapchainImages.size()));

   std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes =
   {
      uniformPoolSize,
      samplerPoolSize
   };

   vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
      .setPoolSizeCount(static_cast<uint32_t>(descriptorPoolSizes.size()))
      .setPPoolSizes(descriptorPoolSizes.data())
      .setMaxSets(static_cast<uint32_t>(swapchainImages.size() * 2));
   descriptorPool = context.device.createDescriptorPool(createInfo);
}

void ForgeApplication::terminateDescriptorPool()
{
   ASSERT(frameDescriptorSets.empty());
   ASSERT(drawDescriptorSets.empty());

   context.device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void ForgeApplication::initializeDescriptorSets()
{
   std::vector<vk::DescriptorSetLayout> frameLayouts(swapchainImages.size(), frameDescriptorSetLayout);
   std::vector<vk::DescriptorSetLayout> drawLayouts(swapchainImages.size(), drawDescriptorSetLayout);

   vk::DescriptorSetAllocateInfo frameAllocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(descriptorPool)
      .setDescriptorSetCount(static_cast<uint32_t>(frameLayouts.size()))
      .setPSetLayouts(frameLayouts.data());
   vk::DescriptorSetAllocateInfo drawAllocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(descriptorPool)
      .setDescriptorSetCount(static_cast<uint32_t>(drawLayouts.size()))
      .setPSetLayouts(drawLayouts.data());

   frameDescriptorSets = context.device.allocateDescriptorSets(frameAllocateInfo);
   drawDescriptorSets = context.device.allocateDescriptorSets(drawAllocateInfo);

   const Texture* texture = textureResourceManager.get(textureHandle);
   ASSERT(texture);

   for (std::size_t i = 0; i < swapchainImages.size(); ++i)
   {
      vk::DescriptorBufferInfo viewBufferInfo = viewUniformBuffer->getDescriptorBufferInfo(context, static_cast<uint32_t>(i));
      vk::DescriptorBufferInfo meshBufferInfo = meshUniformBuffer->getDescriptorBufferInfo(context, static_cast<uint32_t>(i));

      vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(texture->getDefaultView())
         .setSampler(sampler);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(frameDescriptorSets[i])
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      vk::WriteDescriptorSet meshDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(drawDescriptorSets[i])
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&meshBufferInfo);

      vk::WriteDescriptorSet imageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(drawDescriptorSets[i])
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo);

      context.device.updateDescriptorSets({ viewDescriptorWrite, meshDescriptorWrite, imageDescriptorWrite }, {});
   }
}

void ForgeApplication::terminateDescriptorSets()
{
   frameDescriptorSets.clear();
   drawDescriptorSets.clear();
}

void ForgeApplication::initializeMesh()
{
   meshHandle = meshResourceManager.load("Resources/Meshes/Viking/viking_room.obj", context);
   if (!meshHandle)
   {
      throw std::runtime_error(std::string("Failed to load mesh"));
   }

   textureHandle = textureResourceManager.load("Resources/Meshes/Viking/viking_room.png", context);
   if (!textureHandle)
   {
      throw std::runtime_error(std::string("Failed to load image"));
   }

   bool anisotropySupported = context.physicalDeviceFeatures.samplerAnisotropy;
   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
      .setAnisotropyEnable(anisotropySupported)
      .setMaxAnisotropy(anisotropySupported ? 16.0f : 1.0f)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(0.0f)
      .setMinLod(0.0f)
      .setMaxLod(16.0f);
   sampler = context.device.createSampler(samplerCreateInfo);
}

void ForgeApplication::terminateMesh()
{
   meshResourceManager.unload(meshHandle);
   meshHandle.reset();

   textureResourceManager.unload(textureHandle);
   textureHandle.reset();

   context.device.destroySampler(sampler);
   sampler = nullptr;
}

void ForgeApplication::initializeCommandBuffers()
{
   if (!commandPool)
   {
      vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
         .setQueueFamilyIndex(context.queueFamilyIndices.graphicsFamily);

      commandPool = context.device.createCommandPool(commandPoolCreateInfo);
   }

   vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
      .setCommandPool(commandPool)
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(static_cast<uint32_t>(swapchainFramebuffers.size()));

   ASSERT(commandBuffers.empty());
   commandBuffers = context.device.allocateCommandBuffers(commandBufferAllocateInfo);

   const Mesh* mesh = meshResourceManager.get(meshHandle);
   ASSERT(mesh);

   for (std::size_t i = 0; i < commandBuffers.size(); ++i)
   {
      vk::CommandBuffer commandBuffer = commandBuffers[i];

      vk::CommandBufferBeginInfo commandBufferBeginInfo;

      commandBuffer.begin(commandBufferBeginInfo);

      std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
      std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(clearColorValues), vk::ClearDepthStencilValue(1.0f, 0) };

      vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
         .setRenderPass(renderPass)
         .setFramebuffer(swapchainFramebuffers[i])
         .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent))
         .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
         .setPClearValues(clearValues.data());

      commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { frameDescriptorSets[i], drawDescriptorSets[i] }, {});

      for (uint32_t section = 0; section < mesh->getNumSections(); ++section)
      {
         mesh->bindBuffers(commandBuffer, section);
         mesh->draw(commandBuffer, section);
      }

      commandBuffer.endRenderPass();

      commandBuffer.end();
   }
}

void ForgeApplication::terminateCommandBuffers(bool keepPoolAlive)
{
   if (keepPoolAlive)
   {
      context.device.freeCommandBuffers(commandPool, commandBuffers);
   }
   else
   {
      // Command buffers will get cleaned up with the pool
      context.device.destroyCommandPool(commandPool);
      commandPool = nullptr;
   }

   commandBuffers.clear();
}

void ForgeApplication::initializeSyncObjects()
{
   imageAvailableSemaphores.resize(kMaxFramesInFlight);
   renderFinishedSemaphores.resize(kMaxFramesInFlight);
   frameFences.resize(kMaxFramesInFlight);
   imageFences.resize(swapchainImages.size());

   vk::SemaphoreCreateInfo semaphoreCreateInfo;
   vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo()
      .setFlags(vk::FenceCreateFlagBits::eSignaled);

   for (std::size_t i = 0; i < kMaxFramesInFlight; ++i)
   {
      imageAvailableSemaphores[i] = context.device.createSemaphore(semaphoreCreateInfo);
      renderFinishedSemaphores[i] = context.device.createSemaphore(semaphoreCreateInfo);
      frameFences[i] = context.device.createFence(fenceCreateInfo);
   }
}

void ForgeApplication::terminateSyncObjects()
{
   for (vk::Fence frameFence : frameFences)
   {
      context.device.destroyFence(frameFence);
   }
   frameFences.clear();

   for (vk::Semaphore renderFinishedSemaphore : renderFinishedSemaphores)
   {
      context.device.destroySemaphore(renderFinishedSemaphore);
   }
   renderFinishedSemaphores.clear();

   for (vk::Semaphore imageAvailableSemaphore : imageAvailableSemaphores)
   {
      context.device.destroySemaphore(imageAvailableSemaphore);
   }
   imageAvailableSemaphores.clear();
}
