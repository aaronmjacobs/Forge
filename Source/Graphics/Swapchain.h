#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <memory>
#include <vector>

class Texture;

struct SwapchainSupportDetails
{
   vk::SurfaceCapabilitiesKHR capabilities;
   std::vector<vk::SurfaceFormatKHR> surfaceFormats;
   std::vector<vk::PresentModeKHR> presentModes;

   static const vk::SurfaceFormatKHR kDefaultSurfaceFormat;

   bool isValid() const
   {
      return !surfaceFormats.empty() && !presentModes.empty();
   }

   bool supportsHDR() const;
   vk::SurfaceFormatKHR chooseSurfaceFormat(bool preferHDR) const;
   vk::PresentModeKHR choosePresentMode(bool limitFrameRate) const;
};

class Swapchain : public GraphicsResource
{
public:
   static SwapchainSupportDetails getSupportDetails(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

   Swapchain(const GraphicsContext& graphicsContext, vk::Extent2D desiredExtent, bool limitFrameRate, bool preferHDR);
   ~Swapchain();

   vk::Format getFormat() const
   {
      return surfaceFormat.format;
   }

   bool isHDR() const;

   bool supportsHDR() const
   {
      return supportsHDRFormat;
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
      return static_cast<uint32_t>(textures.size());
   }

   Texture& getCurrentTexture() const;

private:
   vk::SurfaceFormatKHR surfaceFormat;
   vk::Extent2D extent;

   uint32_t minImageCount = 0;
   vk::SwapchainKHR swapchainKHR;
   std::vector<std::unique_ptr<Texture>> textures;

   bool supportsHDRFormat = false;
};
