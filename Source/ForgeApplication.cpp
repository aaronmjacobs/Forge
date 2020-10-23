#include "ForgeApplication.h"

#include "Core/Log.h"

#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PlatformUtils/IOUtils.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

   vk::CommandBuffer beginSingleTimeCommands(const VulkanContext& context)
   {
      vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
         .setLevel(vk::CommandBufferLevel::ePrimary)
         .setCommandPool(context.transientCommandPool)
         .setCommandBufferCount(1);

      std::vector<vk::CommandBuffer> commandBuffers = context.device.allocateCommandBuffers(commandBufferAllocateInfo);
      ASSERT(commandBuffers.size() == 1);
      vk::CommandBuffer commandBuffer = commandBuffers[0];

      vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      commandBuffer.begin(beginInfo);

      return commandBuffer;
   }

   void endSingleTimeCommands(const VulkanContext& context, vk::CommandBuffer commandBuffer)
   {
      commandBuffer.end();

      vk::SubmitInfo submitInfo = vk::SubmitInfo()
         .setCommandBufferCount(1)
         .setPCommandBuffers(&commandBuffer);

      context.graphicsQueue.submit({ submitInfo }, nullptr);
      context.graphicsQueue.waitIdle();

      context.device.freeCommandBuffers(context.transientCommandPool, commandBuffer);
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
      vk::CommandBuffer copyCommandBuffer = beginSingleTimeCommands(context);

      for (const BufferCopyInfo& info : copyInfo)
      {
         vk::BufferCopy copyRegion = vk::BufferCopy()
            .setSrcOffset(info.srcOffset)
            .setDstOffset(info.dstOffset)
            .setSize(info.size);
         copyCommandBuffer.copyBuffer(info.srcBuffer, info.dstBuffer, { copyRegion });
      }

      endSingleTimeCommands(context, copyCommandBuffer);
   }

   void copyBufferToImage(const VulkanContext& context, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
   {
      vk::CommandBuffer commandBuffer = beginSingleTimeCommands(context);

      vk::ImageSubresourceLayers imageSubresource = vk::ImageSubresourceLayers()
         .setAspectMask(vk::ImageAspectFlagBits::eColor)
         .setMipLevel(0)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::BufferImageCopy region = vk::BufferImageCopy()
         .setBufferOffset(0)
         .setBufferRowLength(0)
         .setBufferImageHeight(0)
         .setImageSubresource(imageSubresource)
         .setImageOffset(vk::Offset3D(0, 0))
         .setImageExtent(vk::Extent3D(width, height, 1));

      commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });

      endSingleTimeCommands(context, commandBuffer);
   }

   void transitionImageLayout(const VulkanContext& context, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
   {
      vk::CommandBuffer commandBuffer = beginSingleTimeCommands(context);

      vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
         .setAspectMask(vk::ImageAspectFlagBits::eColor)
         .setBaseMipLevel(0)
         .setLevelCount(1)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::AccessFlags srcAccessMask;
      vk::AccessFlags dstAccessMask;
      vk::PipelineStageFlags srcStage;
      vk::PipelineStageFlags dstStage;
      if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
      {
         dstAccessMask = vk::AccessFlagBits::eTransferWrite;

         srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
         dstStage = vk::PipelineStageFlagBits::eTransfer;
      }
      else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
      {
         srcAccessMask = vk::AccessFlagBits::eTransferWrite;
         dstAccessMask = vk::AccessFlagBits::eShaderRead;

         srcStage = vk::PipelineStageFlagBits::eTransfer;
         dstStage = vk::PipelineStageFlagBits::eFragmentShader;

      }
      else
      {
         throw std::runtime_error("Unsupported image layout transition");
      }

      vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
         .setOldLayout(oldLayout)
         .setNewLayout(newLayout)
         .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
         .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
         .setImage(image)
         .setSubresourceRange(subresourceRange)
         .setSrcAccessMask(srcAccessMask)
         .setDstAccessMask(dstAccessMask);

      commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), nullptr, nullptr, { barrier });

      endSingleTimeCommands(context, commandBuffer);
   }

   vk::ImageView createImageView(const VulkanContext& context, vk::Image image, vk::Format format)
   {
      vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
         .setAspectMask(vk::ImageAspectFlagBits::eColor)
         .setBaseMipLevel(0)
         .setLevelCount(1)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
         .setImage(image)
         .setViewType(vk::ImageViewType::e2D)
         .setFormat(format)
         .setSubresourceRange(subresourceRange);

      return context.device.createImageView(createInfo);
   }

   vk::DeviceSize getAlignedSize(vk::DeviceSize size, vk::DeviceSize alignment)
   {
      ASSERT((alignment & (alignment - 1)) == 0, "Alignment is not a power of two: %llu", alignment);
      return (size + (alignment - 1)) & ~(alignment - 1);
   }

   struct ViewUniformData
   {
      alignas(16) glm::mat4 worldToClip;
   };

   struct MeshUniformData
   {
      alignas(16) glm::mat4 localToWorld;
   };

   struct UniformBufferData
   {
      ViewUniformData view;
      MeshUniformData mesh;

      static vk::DeviceSize getPaddedSize(const VulkanContext& context)
      {
         vk::DeviceSize alignment = context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
         vk::DeviceSize paddedViewSize = getAlignedSize(static_cast<vk::DeviceSize>(sizeof(ViewUniformData)), alignment);
         vk::DeviceSize paddedMeshSize = getAlignedSize(static_cast<vk::DeviceSize>(sizeof(MeshUniformData)), alignment);

         return paddedViewSize + paddedMeshSize;
      }

      static vk::DeviceSize getPaddedMeshDataOffset(const VulkanContext& context)
      {
         vk::DeviceSize alignment = context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
         vk::DeviceSize paddedViewSize = getAlignedSize(static_cast<vk::DeviceSize>(sizeof(ViewUniformData)), alignment);

         return paddedViewSize;
      }

      void copyToDeviceMemory(const VulkanContext& context, void* memory) const
      {
         std::memcpy(memory, &view, sizeof(ViewUniformData));
         std::memcpy(static_cast<char*>(memory) + getPaddedMeshDataOffset(context), &mesh, sizeof(MeshUniformData));
      }
   };

   struct ImageDataDeleter
   {
      void operator()(uint8_t* data) const
      {
         stbi_image_free(data);
      }
   };

   struct LoadedImage
   {
      std::unique_ptr<uint8_t, ImageDataDeleter> data;
      vk::Format format = vk::Format::eR8G8B8A8Srgb;
      vk::DeviceSize size = 0;
      uint32_t width = 0;
      uint32_t height = 0;
   };

   std::optional<LoadedImage> loadImage(const std::filesystem::path& path)
   {
      if (std::optional<std::vector<uint8_t>> imageData = IOUtils::readBinaryFile(path))
      {
         int textureWidth = 0;
         int textureHeight = 0;
         int textureChannels = 0;
         stbi_uc* pixelData = stbi_load_from_memory(imageData->data(), static_cast<int>(imageData->size()), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

         if (pixelData)
         {
            LoadedImage loadedImage;
            loadedImage.data.reset(pixelData);
            loadedImage.format = vk::Format::eR8G8B8A8Srgb;
            loadedImage.size = static_cast<vk::DeviceSize>(textureWidth * textureHeight * STBI_rgb_alpha);
            loadedImage.width = static_cast<uint32_t>(textureWidth);
            loadedImage.height = static_cast<uint32_t>(textureHeight);

            return loadedImage;
         }
      }

      return {};
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
std::array<vk::VertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
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
         .setOffset(offsetof(Vertex, color)),
      vk::VertexInputAttributeDescription()
         .setLocation(2)
         .setBinding(0)
         .setFormat(vk::Format::eR32G32Sfloat)
         .setOffset(offsetof(Vertex, texCoord))
   };
}

void Mesh::initialize(const VulkanContext& context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
   vk::DeviceSize vertexDataSize = vertices.size() * sizeof(Vertex);
   vk::DeviceSize indexDataSize = indices.size() * sizeof(uint32_t);

   // Create the buffer

   vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
      .setSize(vertexDataSize + indexDataSize)
      .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer)
      .setSharingMode(vk::SharingMode::eExclusive);
   buffer = context.device.createBuffer(bufferCreateInfo);

   vk::MemoryRequirements memoryRequirements = context.device.getBufferMemoryRequirements(buffer);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(findMemoryType(context.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

   deviceMemory = context.device.allocateMemory(allocateInfo);
   context.device.bindBufferMemory(buffer, deviceMemory, 0);

   indexOffset = vertexDataSize;
   numIndices = static_cast<uint32_t>(indices.size());

   // Create staging buffer, and use it to copy over data

   vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo()
      .setSize(bufferCreateInfo.size)
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
   std::memcpy(mappedData, vertices.data(), static_cast<std::size_t>(vertexDataSize));
   std::memcpy(static_cast<uint8_t*>(mappedData) + vertexDataSize, indices.data(), static_cast<std::size_t>(indexDataSize));
   context.device.unmapMemory(stagingDeviceMemory);
   mappedData = nullptr;

   BufferCopyInfo copyInfo;
   copyInfo.srcBuffer = stagingBuffer;
   copyInfo.dstBuffer = buffer;
   copyInfo.size = vertexDataSize + indexDataSize;

   copyBuffers(context, { copyInfo });

   context.device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;

   context.device.freeMemory(stagingDeviceMemory);
   stagingDeviceMemory = nullptr;
}

void Mesh::terminate(const VulkanContext& context)
{
   context.device.destroyBuffer(buffer);
   buffer = nullptr;

   context.device.freeMemory(deviceMemory);
   deviceMemory = nullptr;

   indexOffset = 0;
   numIndices = 0;
}

ForgeApplication::ForgeApplication()
{
   initializeGlfw();
   initializeVulkan();
   initializeSwapchain();
   initializeRenderPass();
   initializeDescriptorSetLayouts();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeTransientCommandPool();
   initializeUniformBuffers();
   initializeTextureImage();
   initializeDescriptorPool();
   initializeDescriptorSets();
   initializeMesh();
   initializeCommandBuffers();
   initializeSyncObjects();
}

ForgeApplication::~ForgeApplication()
{
   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateMesh();
   terminateDescriptorSets();
   terminateDescriptorPool();
   terminateTextureImage();
   terminateUniformBuffers();
   terminateFramebuffers();
   terminateTransientCommandPool();
   terminateGraphicsPipeline();
   terminateDescriptorSetLayouts();
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
   UniformBufferData uniformBufferData;

   double time = glfwGetTime();

   glm::mat4 worldToView = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   glm::mat4 viewToClip = glm::perspective(glm::radians(45.0f), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, 0.1f, 10.0f);
   viewToClip[1][1] *= -1.0f;

   uniformBufferData.view.worldToClip = viewToClip * worldToView;

   uniformBufferData.mesh.localToWorld = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * static_cast<float>(time), glm::vec3(0.0f, 0.0f, 1.0f));

   vk::DeviceSize size = UniformBufferData::getPaddedSize(context);
   vk::DeviceSize offset = size * index;
   void* memory = context.device.mapMemory(uniformBufferMemory, offset, size);
   uniformBufferData.copyToDeviceMemory(context, memory);
   context.device.unmapMemory(uniformBufferMemory);
   memory = nullptr;
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
   deviceFeatures.setSamplerAnisotropy(context.physicalDeviceFeatures.samplerAnisotropy);

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
      swapchainImageViews.push_back(createImageView(context, swapchainImage, swapchainImageFormat));
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

   std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts = { frameDescriptorSetLayout, drawDescriptorSetLayout };
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayouts.size()))
      .setPSetLayouts(descriptorSetLayouts.data());
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

   graphicsPipeline = context.device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo).value;

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

void ForgeApplication::initializeUniformBuffers()
{
   vk::DeviceSize bufferSize = UniformBufferData::getPaddedSize(context) * swapchainImages.size();
   createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers, uniformBufferMemory);

   void* memory = context.device.mapMemory(uniformBufferMemory, 0, bufferSize);
   std::memset(memory, 0, bufferSize);
   context.device.unmapMemory(uniformBufferMemory);
   memory = nullptr;
}

void ForgeApplication::terminateUniformBuffers()
{
   context.device.destroyBuffer(uniformBuffers);
   uniformBuffers = nullptr;

   context.device.freeMemory(uniformBufferMemory);
   uniformBufferMemory = nullptr;
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

   vk::DeviceSize uniformBufferDataSize = UniformBufferData::getPaddedSize(context);
   vk::DeviceSize paddedMeshDataOffset = UniformBufferData::getPaddedMeshDataOffset(context);
   for (std::size_t i = 0; i < swapchainImages.size(); ++i)
   {
      vk::DescriptorBufferInfo viewBufferInfo = vk::DescriptorBufferInfo()
         .setBuffer(uniformBuffers)
         .setOffset(static_cast<vk::DeviceSize>(uniformBufferDataSize * i))
         .setRange(sizeof(ViewUniformData));

      vk::DescriptorBufferInfo meshBufferInfo = vk::DescriptorBufferInfo()
         .setBuffer(uniformBuffers)
         .setOffset(static_cast<vk::DeviceSize>(uniformBufferDataSize * i + paddedMeshDataOffset))
         .setRange(sizeof(MeshUniformData));

      vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(textureImageView)
         .setSampler(textureImageSampler);

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

void ForgeApplication::initializeTextureImage()
{
   std::optional<LoadedImage> image;
   if (std::optional<std::filesystem::path> absoluteImagePath = IOUtils::getAboluteProjectPath("Resources/Textures/statue.jpg"))
   {
      image = loadImage(*absoluteImagePath);
   }

   if (!image)
   {
      throw std::runtime_error(std::string("Failed to load texture image"));
   }

   vk::Buffer stagingBuffer;
   vk::DeviceMemory stagingBufferMemory;
   createBuffer(image->size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

   void* memory = context.device.mapMemory(stagingBufferMemory, 0, image->size);
   std::memcpy(memory, image->data.get(), static_cast<std::size_t>(image->size));
   context.device.unmapMemory(stagingBufferMemory);
   memory = nullptr;

   image->data.reset();

   createImage(image->width, image->height, image->format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

   transitionImageLayout(context, textureImage, image->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
   copyBufferToImage(context, stagingBuffer, textureImage, image->width, image->height);
   transitionImageLayout(context, textureImage, image->format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

   context.device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;
   context.device.freeMemory(stagingBufferMemory);
   stagingBufferMemory = nullptr;

   textureImageView = createImageView(context, textureImage, image->format);

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
      .setMaxLod(0.0f);
   textureImageSampler = context.device.createSampler(samplerCreateInfo);
}

void ForgeApplication::terminateTextureImage()
{
   context.device.destroySampler(textureImageSampler);
   textureImageSampler = nullptr;

   context.device.destroyImageView(textureImageView);
   textureImageView = nullptr;

   context.device.destroyImage(textureImage);
   textureImage = nullptr;

   context.device.freeMemory(textureImageMemory);
   textureImageMemory = nullptr;
}

void ForgeApplication::initializeMesh()
{
   std::vector<Vertex> vertices =
   {
      Vertex(glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
      Vertex(glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
      Vertex(glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
      Vertex(glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
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

      commandBuffer.bindVertexBuffers(0, { quadMesh.buffer }, { 0 });
      commandBuffer.bindIndexBuffer(quadMesh.buffer, quadMesh.indexOffset, vk::IndexType::eUint32);

      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { frameDescriptorSets[i], drawDescriptorSets[i] }, {});

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

void ForgeApplication::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory) const
{
   vk::ImageCreateInfo imageCreateinfo = vk::ImageCreateInfo()
      .setImageType(vk::ImageType::e2D)
      .setExtent(vk::Extent3D(width, height, 1))
      .setMipLevels(1)
      .setArrayLayers(1)
      .setFormat(format)
      .setTiling(vk::ImageTiling::eOptimal)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setUsage(usage)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setSamples(vk::SampleCountFlagBits::e1);
   image = context.device.createImage(imageCreateinfo);

   vk::MemoryRequirements memoryRequirements = context.device.getImageMemoryRequirements(image);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(findMemoryType(context.physicalDevice, memoryRequirements.memoryTypeBits, properties));
   imageMemory = context.device.allocateMemory(allocateInfo);
   context.device.bindImageMemory(image, imageMemory, 0);
}
