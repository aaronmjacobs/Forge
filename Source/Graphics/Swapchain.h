#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <memory>
#include <optional>
#include <vector>

class Texture;

struct SwapchainSettings
{
   bool limitFrameRate = true;
   bool preferHDR = false;

   std::optional<vk::PresentModeKHR> desiredPresentMode;
   std::optional<vk::Format> desiredFormat;
   std::optional<vk::ColorSpaceKHR> desiredColorSpace;

   bool operator==(const SwapchainSettings& other) const = default;
};

class Swapchain : public GraphicsResource
{
public:
   Swapchain(const GraphicsContext& graphicsContext, vk::Extent2D desiredExtent, const SwapchainSettings& settings);
   ~Swapchain();

   const SwapchainCapabilities& getCapabilities() const
   {
      return capabilities;
   }

   vk::PresentModeKHR getPresentMode() const
   {
      return presentMode;
   }

   const vk::SurfaceFormatKHR& getSurfaceFormat() const
   {
      return surfaceFormat;
   }

   bool supportsHDR() const;

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
   SwapchainCapabilities capabilities;

   vk::PresentModeKHR presentMode;
   vk::SurfaceFormatKHR surfaceFormat;
   vk::Extent2D extent;

   uint32_t minImageCount = 0;
   vk::SwapchainKHR swapchainKHR;
   std::vector<std::unique_ptr<Texture>> textures;
};
