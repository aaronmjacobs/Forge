#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <vector>

struct SwapchainSupportDetails
{
   vk::SurfaceCapabilitiesKHR capabilities;
   std::vector<vk::SurfaceFormatKHR> formats;
   std::vector<vk::PresentModeKHR> presentModes;

   bool isValid() const
   {
      return !formats.empty() && !presentModes.empty();
   }
};

class Swapchain : public GraphicsResource
{
public:
   static SwapchainSupportDetails getSupportDetails(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

   Swapchain(const GraphicsContext& graphicsContext, vk::Extent2D desiredExtent);

   ~Swapchain();

   vk::Format getFormat() const
   {
      return format;
   }

   const vk::Extent2D& getExtent() const
   {
      return extent;
   }

   vk::SwapchainKHR getSwapchainKHR() const
   {
      return swapchainKHR;
   }

   uint32_t getMinImageCount() const
   {
      return minImageCount;
   }

   uint32_t getImageCount() const
   {
      return static_cast<uint32_t>(images.size());
   }

   const std::vector<vk::ImageView>& getImageViews() const
   {
      return imageViews;
   }

   TextureInfo getTextureInfo() const;

private:
   vk::Format format;
   vk::Extent2D extent;

   uint32_t minImageCount = 0;
   vk::SwapchainKHR swapchainKHR;
   std::vector<vk::Image> images;
   std::vector<vk::ImageView> imageViews;
};
