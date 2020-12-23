#include "Graphics/Swapchain.h"

namespace
{
   vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
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

   vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
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

   vk::ImageView createImageView(const GraphicsContext& context, vk::Image image, vk::Format format)
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

      return context.getDevice().createImageView(createInfo);
   }
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

Swapchain::Swapchain(const GraphicsContext& context, vk::Extent2D desiredExtent)
   : GraphicsResource(context)
{
   SwapchainSupportDetails supportDetails = getSupportDetails(context.getPhysicalDevice(), context.getSurface());
   ASSERT(supportDetails.isValid());

   vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(supportDetails.formats);
   vk::PresentModeKHR presentMode = choosePresentMode(supportDetails.presentModes);

   format = surfaceFormat.format;
   extent = chooseExtent(supportDetails.capabilities, desiredExtent);

   uint32_t desiredMinImageCount = supportDetails.capabilities.minImageCount + 1;
   uint32_t minImageCount = supportDetails.capabilities.maxImageCount > 0 ? std::min(supportDetails.capabilities.maxImageCount, desiredMinImageCount) : desiredMinImageCount;

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
   images = context.getDevice().getSwapchainImagesKHR(swapchainKHR);

   imageViews.reserve(images.size());
   for (vk::Image image : images)
   {
      imageViews.push_back(createImageView(context, image, format));
   }
}

Swapchain::~Swapchain()
{
   for (vk::ImageView imageView : imageViews)
   {
      device.destroyImageView(imageView);
   }

   device.destroySwapchainKHR(swapchainKHR);
}
