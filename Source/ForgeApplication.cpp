#include "ForgeApplication.h"

#include "Core/Log.h"

#include "Graphics/Command.h"
#include "Graphics/Memory.h"
#include "Graphics/Swapchain.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PlatformUtils/IOUtils.h>

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
   initializeShaders();
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
   terminateShaders();
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
      if (!recreateSwapchain())
      {
         return;
      }
   }

   vk::Result frameWaitResult = context.device.waitForFences({ frameFences[frameIndex] }, true, UINT64_MAX);
   if (frameWaitResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to wait for frame fence");
   }

   vk::ResultValue<uint32_t> imageIndexResultValue = context.device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   if (imageIndexResultValue.result == vk::Result::eErrorOutOfDateKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }

      imageIndexResultValue = context.device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
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
      .setWaitSemaphores(waitSemaphores)
      .setPWaitDstStageMask(waitStages.data())
      .setCommandBuffers(commandBuffers[imageIndex])
      .setSignalSemaphores(signalSemaphores);

   context.device.resetFences({ frameFences[frameIndex] });
   context.graphicsQueue.submit({ submitInfo }, frameFences[frameIndex]);

   vk::SwapchainKHR swapchainKHR = swapchain->getSwapchainKHR();
   vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
      .setWaitSemaphores(signalSemaphores)
      .setSwapchains(swapchainKHR)
      .setImageIndices(imageIndex);

   vk::Result presentResult = context.presentQueue.presentKHR(presentInfo);
   if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }
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

   vk::Extent2D swapchainExtent = swapchain->getExtent();
   glm::mat4 viewToClip = glm::perspective(glm::radians(70.0f), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, 0.1f, 10.0f);
   viewToClip[1][1] *= -1.0f;

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;

   MeshUniformData meshUniformData;
   meshUniformData.localToWorld = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * static_cast<float>(time), glm::vec3(0.0f, 0.0f, 1.0f));

   viewUniformBuffer->update(context, viewUniformData, index);
   meshUniformBuffer->update(context, meshUniformData, index);
}

bool ForgeApplication::recreateSwapchain()
{
   context.device.waitIdle();

   terminateCommandBuffers(true);
   terminateDescriptorSets();
   terminateDescriptorPool();
   terminateUniformBuffers();
   terminateFramebuffers();
   terminateGraphicsPipeline();
   terminateRenderPass();
   terminateSwapchain();

   vk::Extent2D windowExtent = getWindowExtent();
   while ((windowExtent.width == 0 || windowExtent.height == 0) && !glfwWindowShouldClose(window))
   {
      glfwWaitEvents();
      windowExtent = getWindowExtent();
   }

   if (windowExtent.width == 0 || windowExtent.height == 0)
   {
      return false;
   }

   initializeSwapchain();
   initializeRenderPass();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeUniformBuffers();
   initializeDescriptorPool();
   initializeDescriptorSets();
   initializeCommandBuffers();

   framebufferResized = false;
   return true;
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
      .setPEnabledExtensionNames(extensions)
      .setPEnabledLayerNames(layers);

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
         .setQueuePriorities(queuePriority));
   }

   std::vector<const char*> deviceExtensions = getDeviceExtensions(context.physicalDevice);

   vk::PhysicalDeviceFeatures deviceFeatures;
   deviceFeatures.setSamplerAnisotropy(context.physicalDeviceFeatures.samplerAnisotropy);
   deviceFeatures.setSampleRateShading(true);

   vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
      .setQueueCreateInfos(deviceQueueCreateInfos)
      .setPEnabledExtensionNames(deviceExtensions)
      .setPEnabledFeatures(&deviceFeatures);

#if FORGE_DEBUG
   deviceCreateInfo = deviceCreateInfo
      .setPEnabledLayerNames(layers);
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
   swapchain = std::make_unique<Swapchain>(context, getWindowExtent());

   vk::Extent2D swapchainExtent = swapchain->getExtent();
   vk::SampleCountFlagBits sampleCount = getMaxSampleCount(context);
   {
      ImageProperties colorImageProperties;
      colorImageProperties.format = swapchain->getFormat();
      colorImageProperties.width = swapchainExtent.width;
      colorImageProperties.height = swapchainExtent.height;

      TextureProperties colorTextureProperties;
      colorTextureProperties.sampleCount = sampleCount;
      colorTextureProperties.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
      colorTextureProperties.aspects = vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout colorInitialLayout;
      colorInitialLayout.layout = vk::ImageLayout::eColorAttachmentOptimal;
      colorInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

      colorTexture = std::make_unique<Texture>(context, colorImageProperties, colorTextureProperties, colorInitialLayout);
   }

   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = Texture::findSupportedFormat(context, { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
      depthImageProperties.width = swapchainExtent.width;
      depthImageProperties.height = swapchainExtent.height;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = sampleCount;
      depthTextureProperties.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
      depthTextureProperties.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

      TextureInitialLayout depthInitialLayout;
      depthInitialLayout.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);

      depthTexture = std::make_unique<Texture>(context, depthImageProperties, depthTextureProperties, depthInitialLayout);
   }
}

void ForgeApplication::terminateSwapchain()
{
   depthTexture = nullptr;
   colorTexture = nullptr;

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
      .setFormat(swapchain->getFormat())
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
      .setAttachments(attachments)
      .setSubpasses(subpassDescription)
      .setDependencies(subpassDependency);

   renderPass = context.device.createRenderPass(renderPassCreateInfo);
}

void ForgeApplication::terminateRenderPass()
{
   context.device.destroyRenderPass(renderPass);
   renderPass = nullptr;
}

void ForgeApplication::initializeShaders()
{
   simpleShader = std::make_unique<SimpleShader>(shaderModuleResourceManager, context);
}

void ForgeApplication::terminateShaders()
{
   simpleShader.reset();
   shaderModuleResourceManager.clear();
}

void ForgeApplication::initializeGraphicsPipeline()
{
   std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = simpleShader->getStages(true);

   std::array<vk::VertexInputBindingDescription, 1> vertexBindingDescriptions = Vertex::getBindingDescriptions();
   std::array<vk::VertexInputAttributeDescription, 3> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
   vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo()
      .setVertexBindingDescriptions(vertexBindingDescriptions)
      .setVertexAttributeDescriptions(vertexAttributeDescriptions);

   vk::Extent2D swapchainExtent = swapchain->getExtent();
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
      .setViewports(viewport)
      .setScissors(scissor);

   vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo()
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0f)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eCounterClockwise);

   vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
      .setRasterizationSamples(colorTexture->getTextureProperties().sampleCount)
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
      .setAttachments(colorBlendAttachmentState);

   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = simpleShader->getSetLayouts();
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
}

void ForgeApplication::terminateGraphicsPipeline()
{
   context.device.destroyPipeline(graphicsPipeline);
   graphicsPipeline = nullptr;

   context.device.destroyPipelineLayout(pipelineLayout);
   pipelineLayout = nullptr;
}

void ForgeApplication::initializeFramebuffers()
{
   vk::Extent2D swapchainExtent = swapchain->getExtent();

   ASSERT(swapchainFramebuffers.empty());
   swapchainFramebuffers.reserve(swapchain->getImageCount());

   for (vk::ImageView swapchainImageView : swapchain->getImageViews())
   {
      std::array<vk::ImageView, 3> attachments = { colorTexture->getDefaultView(), depthTexture->getDefaultView(), swapchainImageView };

      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachments(attachments)
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
   uint32_t swapchainImageCount = swapchain->getImageCount();

   viewUniformBuffer = std::make_unique<UniformBuffer<ViewUniformData>>(context, swapchainImageCount);
   meshUniformBuffer = std::make_unique<UniformBuffer<MeshUniformData>>(context, swapchainImageCount);
}

void ForgeApplication::terminateUniformBuffers()
{
   meshUniformBuffer.reset();
   viewUniformBuffer.reset();
}

void ForgeApplication::initializeDescriptorPool()
{
   uint32_t swapchainImageCount = swapchain->getImageCount();

   vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(swapchainImageCount * 2);

   vk::DescriptorPoolSize samplerPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(swapchainImageCount);

   std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes =
   {
      uniformPoolSize,
      samplerPoolSize
   };

   vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
      .setPoolSizes(descriptorPoolSizes)
      .setMaxSets(swapchainImageCount * 2);
   descriptorPool = context.device.createDescriptorPool(createInfo);
}

void ForgeApplication::terminateDescriptorPool()
{
   ASSERT(!simpleShader->areDescriptorSetsAllocated());

   context.device.destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void ForgeApplication::initializeDescriptorSets()
{
   uint32_t swapchainImageCount = swapchain->getImageCount();
   simpleShader->allocateDescriptorSets(descriptorPool, swapchainImageCount);
   simpleShader->updateDescriptorSets(context, swapchainImageCount, *viewUniformBuffer, *meshUniformBuffer, *textureResourceManager.get(textureHandle), sampler);
}

void ForgeApplication::terminateDescriptorSets()
{
   simpleShader->clearDescriptorSets();
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
         .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), swapchain->getExtent()))
         .setClearValues(clearValues);

      commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

      simpleShader->bindDescriptorSets(commandBuffer, pipelineLayout, static_cast<uint32_t>(i));

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
   imageFences.resize(swapchain->getImageCount());

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

vk::Extent2D ForgeApplication::getWindowExtent() const
{
   int width = 0;
   int height = 0;
   glfwGetFramebufferSize(window, &width, &height);

   return vk::Extent2D(static_cast<uint32_t>(std::max(width, 0)), static_cast<uint32_t>(std::max(height, 0)));
}
