#include "ForgeApplication.h"

#include "Core/Log.h"

#include <GLFW/glfw3.h>
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
   const int kInitialWindowWidth = 800;
   const int kInitialWindowHeight = 600;
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

   QueueFamilyIndices getQueueFamilyIndices(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
   {
      QueueFamilyIndices queueFamilyIndices;

      uint32_t index = 0;
      for (vk::QueueFamilyProperties queueFamilyProperties : physicalDevice.getQueueFamilyProperties())
      {
         if (!queueFamilyIndices.graphicsFamily && (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics))
         {
            queueFamilyIndices.graphicsFamily = index;
         }

         if (!queueFamilyIndices.presentFamily && physicalDevice.getSurfaceSupportKHR(index, surface))
         {
            queueFamilyIndices.presentFamily = index;
         }

         if (queueFamilyIndices.isComplete())
         {
            break;
         }

         ++index;
      }

      return queueFamilyIndices;
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

      QueueFamilyIndices queueFamilyIndices = getQueueFamilyIndices(physicalDevice, surface);
      if (!queueFamilyIndices.isComplete())
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

   vk::ShaderModule createShaderModule(vk::Device device, const std::filesystem::path& relativeShaderPath)
   {
      vk::ShaderModule shaderModule;

      if (std::optional<std::filesystem::path> absoluteShaderPath = IOUtils::getAboluteProjectPath(relativeShaderPath))
      {
         if (std::optional<std::vector<uint8_t>> shaderData = IOUtils::readBinaryFile(*absoluteShaderPath))
         {
            vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
               .setCodeSize(shaderData->size())
               .setPCode(reinterpret_cast<const uint32_t*>(shaderData->data()));

            shaderModule = device.createShaderModule(createInfo);
         }
      }

      return shaderModule;
   }

   uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
   {
      vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();

      for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
      {
         if ((typeFilter & (1 << i))
            && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
         {
            return i;
         }
      }

      throw std::runtime_error("Failed to find suitable memory type");
   }

   struct BufferCopyInfo
   {
      vk::Buffer srcBuffer;
      vk::Buffer dstBuffer;

      vk::DeviceSize srcOffset = 0;
      vk::DeviceSize dstOffset = 0;

      vk::DeviceSize size = 0;
   };

   void copyBuffers(const VulkanContext& context, const std::vector<BufferCopyInfo>& copyInfo)
   {
      vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
         .setLevel(vk::CommandBufferLevel::ePrimary)
         .setCommandPool(context.transientCommandPool)
         .setCommandBufferCount(1);

      std::vector<vk::CommandBuffer> copyCommandBuffers = context.device.allocateCommandBuffers(commandBufferAllocateInfo);
      vk::CommandBuffer copyCommandBuffer = copyCommandBuffers[0];

      vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      copyCommandBuffer.begin(beginInfo);

      for (const BufferCopyInfo& info : copyInfo)
      {
         vk::BufferCopy copyRegion = vk::BufferCopy()
            .setSrcOffset(info.srcOffset)
            .setDstOffset(info.dstOffset)
            .setSize(info.size);
         copyCommandBuffer.copyBuffer(info.srcBuffer, info.dstBuffer, { copyRegion });
      }

      copyCommandBuffer.end();

      vk::SubmitInfo submitInfo = vk::SubmitInfo()
         .setCommandBufferCount(1)
         .setPCommandBuffers(&copyCommandBuffer);

      context.graphicsQueue.submit({ submitInfo }, nullptr);
      context.graphicsQueue.waitIdle();

      context.device.freeCommandBuffers(context.transientCommandPool, copyCommandBuffers);
   }
}

// static
vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
   return vk::VertexInputBindingDescription()
      .setBinding(0)
      .setStride(sizeof(Vertex))
      .setInputRate(vk::VertexInputRate::eVertex);
}

// static
std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()
{
   return
   {
      vk::VertexInputAttributeDescription()
         .setLocation(0)
         .setBinding(0)
         .setFormat(vk::Format::eR32G32Sfloat)
         .setOffset(offsetof(Vertex, position)),
      vk::VertexInputAttributeDescription()
         .setLocation(1)
         .setBinding(0)
         .setFormat(vk::Format::eR32G32B32Sfloat)
         .setOffset(offsetof(Vertex, color))
   };
}

void Mesh::initialize(const VulkanContext& context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
   vk::DeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
   vk::DeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

   // Create vertex and index buffers

   vk::BufferCreateInfo vertexBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(vertexBufferSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
      .setSharingMode(vk::SharingMode::eExclusive);
   vertexBuffer = context.device.createBuffer(vertexBufferCreateInfo);

   vk::BufferCreateInfo indexBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(indexBufferSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
      .setSharingMode(vk::SharingMode::eExclusive);
   indexBuffer = context.device.createBuffer(indexBufferCreateInfo);

   vk::MemoryRequirements vertexMemoryRequirements = context.device.getBufferMemoryRequirements(vertexBuffer);
   vk::MemoryRequirements indexMemoryRequirements = context.device.getBufferMemoryRequirements(vertexBuffer);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(vertexMemoryRequirements.size + indexMemoryRequirements.size)
      .setMemoryTypeIndex(findMemoryType(context.physicalDevice, vertexMemoryRequirements.memoryTypeBits & indexMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

   deviceMemory = context.device.allocateMemory(allocateInfo);
   context.device.bindBufferMemory(vertexBuffer, deviceMemory, 0);
   context.device.bindBufferMemory(indexBuffer, deviceMemory, vertexMemoryRequirements.size);

   numIndices = static_cast<uint32_t>(indices.size());

   // Create staging buffer, and use it to copy over data

   vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(vertexBufferSize + indexBufferSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
      .setSharingMode(vk::SharingMode::eExclusive);
   vk::Buffer stagingBuffer = context.device.createBuffer(stagingBufferCreateInfo);

   vk::MemoryRequirements stagingMemoryRequirements = context.device.getBufferMemoryRequirements(stagingBuffer);
   vk::MemoryAllocateInfo stagingAllocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(stagingMemoryRequirements.size)
      .setMemoryTypeIndex(findMemoryType(context.physicalDevice, stagingMemoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

   vk::DeviceMemory stagingDeviceMemory = context.device.allocateMemory(stagingAllocateInfo);
   context.device.bindBufferMemory(stagingBuffer, stagingDeviceMemory, 0);

   void* mappedData = context.device.mapMemory(stagingDeviceMemory, 0, stagingAllocateInfo.allocationSize, vk::MemoryMapFlags());
   std::memcpy(mappedData, vertices.data(), static_cast<std::size_t>(vertexBufferSize));
   std::memcpy(static_cast<uint8_t*>(mappedData) + vertexBufferSize, indices.data(), static_cast<std::size_t>(indexBufferSize));
   context.device.unmapMemory(stagingDeviceMemory);
   mappedData = nullptr;

   BufferCopyInfo vertexCopyInfo;
   vertexCopyInfo.srcBuffer = stagingBuffer;
   vertexCopyInfo.dstBuffer = vertexBuffer;
   vertexCopyInfo.size = vertexBufferSize;

   BufferCopyInfo indexCopyInfo;
   indexCopyInfo.srcBuffer = stagingBuffer;
   indexCopyInfo.dstBuffer = indexBuffer;
   indexCopyInfo.srcOffset = vertexBufferSize;
   indexCopyInfo.size = indexBufferSize;

   copyBuffers(context, { vertexCopyInfo, indexCopyInfo });

   context.device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;

   context.device.freeMemory(stagingDeviceMemory);
   stagingDeviceMemory = nullptr;
}

void Mesh::terminate(const VulkanContext& context)
{
   context.device.destroyBuffer(vertexBuffer);
   vertexBuffer = nullptr;

   context.device.destroyBuffer(indexBuffer);
   indexBuffer = nullptr;

   context.device.freeMemory(deviceMemory);
   deviceMemory = nullptr;

   numIndices = 0;
}

ForgeApplication::ForgeApplication()
{
   initializeGlfw();
   initializeVulkan();
   initializeSwapchain();
   initializeRenderPass();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeTransientCommandPool();
   initializeMesh();
   initializeCommandBuffers();
   initializeSyncObjects();
}

ForgeApplication::~ForgeApplication()
{
   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateMesh();
   terminateFramebuffers();
   terminateTransientCommandPool();
   terminateGraphicsPipeline();
   terminateRenderPass();
   terminateSwapchain();
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

   context.device.waitForFences({ frameFences[frameIndex] }, true, UINT64_MAX);

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
      context.device.waitForFences({ imageFences[imageIndex] }, true, UINT64_MAX);
   }
   imageFences[imageIndex] = frameFences[frameIndex];

   std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[frameIndex] };
   std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
   static_assert(waitSemaphores.size() == waitStages.size(), "Wait semaphores and wait stages must be parallel arrays");

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
   terminateFramebuffers();
   terminateGraphicsPipeline();
   terminateRenderPass();
   terminateSwapchain();

   initializeSwapchain();
   initializeRenderPass();
   initializeGraphicsPipeline();
   initializeFramebuffers();
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

   context.queueFamilyIndices = getQueueFamilyIndices(context.physicalDevice, context.surface);
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

   context.graphicsQueue = context.device.getQueue(*context.queueFamilyIndices.graphicsFamily, 0);
   context.presentQueue = context.device.getQueue(*context.queueFamilyIndices.presentFamily, 0);
}

void ForgeApplication::terminateVulkan()
{
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

   if (*context.queueFamilyIndices.graphicsFamily == *context.queueFamilyIndices.presentFamily)
   {
      swapchainCreateInfo = swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
   }
   else
   {
      std::array<uint32_t, 2> indices = { *context.queueFamilyIndices.graphicsFamily, *context.queueFamilyIndices.presentFamily };

      swapchainCreateInfo = swapchainCreateInfo
         .setImageSharingMode(vk::SharingMode::eConcurrent)
         .setQueueFamilyIndexCount(static_cast<uint32_t>(indices.size()))
         .setPQueueFamilyIndices(indices.data());
   }

   swapchain = context.device.createSwapchainKHR(swapchainCreateInfo);
   swapchainImages = context.device.getSwapchainImagesKHR(swapchain);
   swapchainImageFormat = surfaceFormat.format;
   swapchainExtent = extent;

   swapchainImageViews.reserve(swapchainImages.size());
   for (vk::Image swapchainImage : swapchainImages)
   {
      vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange()
         .setAspectMask(vk::ImageAspectFlagBits::eColor)
         .setBaseMipLevel(0)
         .setLevelCount(1)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo()
         .setImage(swapchainImage)
         .setViewType(vk::ImageViewType::e2D)
         .setFormat(swapchainImageFormat)
         .setSubresourceRange(imageSubresourceRange);

      swapchainImageViews.push_back(context.device.createImageView(imageViewCreateInfo));
   }
}

void ForgeApplication::terminateSwapchain()
{
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
      .setFormat(swapchainImageFormat)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

   vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
      .setAttachment(0)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

   vk::SubpassDescription subpassDescription = vk::SubpassDescription()
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&colorAttachmentReference);

   vk::SubpassDependency subpassDependency = vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask({})
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

   vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
      .setAttachmentCount(1)
      .setPAttachments(&colorAttachment)
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

void ForgeApplication::initializeGraphicsPipeline()
{
   vk::ShaderModule vertShaderModule = createShaderModule(context.device, "Resources/Shaders/Triangle.vert.spv");
   vk::ShaderModule fragShaderModule = createShaderModule(context.device, "Resources/Shaders/Triangle.frag.spv");

   vk::PipelineShaderStageCreateInfo vertShaderStageCreateinfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vertShaderModule)
      .setPName("main");

   vk::PipelineShaderStageCreateInfo fragShaderStageCreateinfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule)
      .setPName("main");

   std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageCreateinfo, fragShaderStageCreateinfo };

   vk::VertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
   std::array<vk::VertexInputAttributeDescription, 2> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
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
      .setFrontFace(vk::FrontFace::eClockwise);

   vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
      .setSampleShadingEnable(false)
      .setRasterizationSamples(vk::SampleCountFlagBits::e1)
      .setMinSampleShading(1.0f);

   vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
      .setLogicOpEnable(false)
      .setAttachmentCount(1)
      .setPAttachments(&colorBlendAttachmentState);

   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
   pipelineLayout = context.device.createPipelineLayout(pipelineLayoutCreateInfo);

   vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
      .setStageCount(static_cast<uint32_t>(shaderStages.size()))
      .setPStages(shaderStages.data())
      .setPVertexInputState(&vertexInputCreateInfo)
      .setPInputAssemblyState(&inputAssemblyCreateInfo)
      .setPViewportState(&viewportStateCreateInfo)
      .setPRasterizationState(&rasterizationStateCreateInfo)
      .setPMultisampleState(&multisampleStateCreateInfo)
      .setPDepthStencilState(nullptr)
      .setPColorBlendState(&colorBlendStateCreateInfo)
      .setPDynamicState(nullptr)
      .setLayout(pipelineLayout)
      .setRenderPass(renderPass)
      .setSubpass(0)
      .setBasePipelineHandle(nullptr)
      .setBasePipelineIndex(-1);

   graphicsPipeline = context.device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo);

   context.device.destroyShaderModule(fragShaderModule);
   context.device.destroyShaderModule(vertShaderModule);
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
   ASSERT(swapchainFramebuffers.empty());
   swapchainFramebuffers.reserve(swapchainImageViews.size());

   for (vk::ImageView swapchainImageView : swapchainImageViews)
   {
      std::array<vk::ImageView, 1> attachments = { swapchainImageView };

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
      .setQueueFamilyIndex(*context.queueFamilyIndices.graphicsFamily)
      .setFlags(vk::CommandPoolCreateFlagBits::eTransient);

   context.transientCommandPool = context.device.createCommandPool(commandPoolCreateInfo);
}

void ForgeApplication::terminateTransientCommandPool()
{
   context.device.destroyCommandPool(context.transientCommandPool);
   context.transientCommandPool = nullptr;
}

void ForgeApplication::initializeMesh()
{
   std::vector<Vertex> vertices =
   {
      Vertex(glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f)),
      Vertex(glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f)),
      Vertex(glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)),
      Vertex(glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f)),
   };

   std::vector<uint32_t> indices =
   {
      0, 1, 2,
      2, 3, 0
   };

   quadMesh.initialize(context, vertices, indices);
}

void ForgeApplication::terminateMesh()
{
   quadMesh.terminate(context);
}

void ForgeApplication::initializeCommandBuffers()
{
   ASSERT(context.queueFamilyIndices.graphicsFamily.has_value());

   if (!commandPool)
   {
      vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
         .setQueueFamilyIndex(*context.queueFamilyIndices.graphicsFamily);

      commandPool = context.device.createCommandPool(commandPoolCreateInfo);
   }

   vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
      .setCommandPool(commandPool)
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(static_cast<uint32_t>(swapchainFramebuffers.size()));

   ASSERT(commandBuffers.empty());
   commandBuffers = context.device.allocateCommandBuffers(commandBufferAllocateInfo);

   for (std::size_t i = 0; i < commandBuffers.size(); ++i)
   {
      vk::CommandBuffer commandBuffer = commandBuffers[i];

      vk::CommandBufferBeginInfo commandBufferBeginInfo;

      commandBuffer.begin(commandBufferBeginInfo);

      std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
      std::array<vk::ClearValue, 1> clearColors = { vk::ClearColorValue(clearColorValues) };

      vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
         .setRenderPass(renderPass)
         .setFramebuffer(swapchainFramebuffers[i])
         .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent))
         .setClearValueCount(static_cast<uint32_t>(clearColors.size()))
         .setPClearValues(clearColors.data());

      commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

      commandBuffer.bindVertexBuffers(0, { quadMesh.vertexBuffer }, { 0 });
      commandBuffer.bindIndexBuffer(quadMesh.indexBuffer, 0, vk::IndexType::eUint32);

      commandBuffer.drawIndexed(quadMesh.numIndices, 1, 0, 0, 0);

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

void ForgeApplication::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) const
{
   vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
      .setSize(size)
      .setUsage(usage)
      .setSharingMode(vk::SharingMode::eExclusive);

   buffer = context.device.createBuffer(bufferCreateInfo);

   vk::MemoryRequirements memoryRequirements = context.device.getBufferMemoryRequirements(buffer);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(findMemoryType(context.physicalDevice, memoryRequirements.memoryTypeBits, properties));

   bufferMemory = context.device.allocateMemory(allocateInfo);
   context.device.bindBufferMemory(buffer, bufferMemory, 0);
}
