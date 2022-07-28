#include "Graphics/Swapchain.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Texture.h"

namespace
{
   vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D desiredExtent)
   {
      if (capabilities.currentExtent.width != UINT32_MAX)
      {
         return capabilities.currentExtent;
      }

      vk::Extent2D extent;
      extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, desiredExtent.width));
      extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, desiredExtent.height));

      return extent;
   }
}

// static
const vk::SurfaceFormatKHR SwapchainSupportDetails::kDefaultSurfaceFormat(vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear);

// static
const vk::SurfaceFormatKHR SwapchainSupportDetails::kHDRSurfaceFormat(vk::Format::eA2R10G10B10UnormPack32, vk::ColorSpaceKHR::eHdr10St2084EXT);

bool SwapchainSupportDetails::supportsHDR() const
{
   for (const vk::SurfaceFormatKHR& format : formats)
   {
      if (format == kHDRSurfaceFormat)
      {
         return true;
      }
   }

   return false;
}

vk::SurfaceFormatKHR SwapchainSupportDetails::chooseSurfaceFormat(bool preferHDR) const
{
   if (preferHDR)
   {
      for (const vk::SurfaceFormatKHR& format : formats)
      {
         if (format == kHDRSurfaceFormat)
         {
            return format;
         }
      }
   }

   for (const vk::SurfaceFormatKHR& format : formats)
   {
      if (format == kDefaultSurfaceFormat)
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

vk::PresentModeKHR SwapchainSupportDetails::choosePresentMode() const
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

// static
SwapchainSupportDetails Swapchain::getSupportDetails(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
   SwapchainSupportDetails supportDetails;

   supportDetails.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
   supportDetails.formats = physicalDevice.getSurfaceFormatsKHR(surface);
   supportDetails.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

   return supportDetails;
}

Swapchain::Swapchain(const GraphicsContext& graphicsContext, vk::Extent2D desiredExtent, bool preferHDR)
   : GraphicsResource(graphicsContext)
{
   SwapchainSupportDetails supportDetails = getSupportDetails(context.getPhysicalDevice(), context.getSurface());
   ASSERT(supportDetails.isValid());

   supportsHDRFormat = supportDetails.supportsHDR();

   vk::SurfaceFormatKHR surfaceFormat = supportDetails.chooseSurfaceFormat(preferHDR);
   vk::PresentModeKHR presentMode = supportDetails.choosePresentMode();

   format = surfaceFormat.format;
   extent = chooseExtent(supportDetails.capabilities, desiredExtent);

#if defined(__APPLE__)
   // Using 3 buffers causes framerate stutter on macOS (see https://github.com/KhronosGroup/MoltenVK/issues/1407)
   uint32_t desiredMinImageCount = supportDetails.capabilities.minImageCount;
#else
   uint32_t desiredMinImageCount = supportDetails.capabilities.minImageCount + 1;
#endif
   minImageCount = supportDetails.capabilities.maxImageCount > 0 ? std::min(supportDetails.capabilities.maxImageCount, desiredMinImageCount) : desiredMinImageCount;

   vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
      .setSurface(context.getSurface())
      .setMinImageCount(minImageCount)
      .setImageFormat(surfaceFormat.format)
      .setImageColorSpace(surfaceFormat.colorSpace)
      .setImageExtent(extent)
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPreTransform(supportDetails.capabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(presentMode)
      .setClipped(true)
      .setImageSharingMode(vk::SharingMode::eExclusive);

   std::array<uint32_t, 2> indices = { context.getQueueFamilyIndices().graphicsFamily, context.getQueueFamilyIndices().presentFamily };
   if (context.getQueueFamilyIndices().graphicsFamily != context.getQueueFamilyIndices().presentFamily)
   {
      createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
      createInfo.setQueueFamilyIndices(indices);
   }

   swapchainKHR = context.getDevice().createSwapchainKHR(createInfo);
   std::vector<vk::Image> images = context.getDevice().getSwapchainImagesKHR(swapchainKHR);

   ImageProperties imageProperties;
   imageProperties.format = surfaceFormat.format;
   imageProperties.width = extent.width;
   imageProperties.height = extent.height;
   imageProperties.hasAlpha = FormatHelpers::hasAlpha(imageProperties.format);

   textures.reserve(images.size());
   uint32_t index = 0;
   for (vk::Image image : images)
   {
      textures.push_back(std::make_unique<Texture>(context, imageProperties, image));
      NAME_CHILD_POINTER(textures[index], "Texture " + DebugUtils::toString(index));
      ++index;
   }
}

Swapchain::~Swapchain()
{
   textures.clear();

   device.destroySwapchainKHR(swapchainKHR);
}

Texture& Swapchain::getCurrentTexture() const
{
   uint32_t index = context.getSwapchainIndex();
   ASSERT(index < textures.size() && textures[index] != nullptr);

   return *textures[index];
}
