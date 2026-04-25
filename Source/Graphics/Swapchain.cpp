#include "Graphics/Swapchain.h"

#include "Core/Log.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/Texture.h"

#include <algorithm>
#include <ranges>

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

   std::vector<vk::ColorSpaceKHR> getColorSpacesForFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats, vk::Format format)
   {
      std::vector<vk::ColorSpaceKHR> colorSpaces;

      for (const vk::SurfaceFormatKHR& surfaceFormat : surfaceFormats)
      {
         if (surfaceFormat.format == format)
         {
            colorSpaces.push_back(surfaceFormat.colorSpace);
         }
      }

      return colorSpaces;
   }

   std::span<const vk::ColorSpaceKHR> getPreferredColorSpacesForFormat(vk::Format format)
   {
      // For 8-bit formats, we just want plain old sRGB
      static const std::array<vk::ColorSpaceKHR, 1> kPreferred8BitColorSpaces =
      {
         vk::ColorSpaceKHR::eSrgbNonlinear
      };

      // For 10-bit formats, prefer HDR10 color spaces
      static const std::array<vk::ColorSpaceKHR, 2> kPreferred10BitColorSpaces =
      {
         vk::ColorSpaceKHR::eHdr10St2084EXT,
         vk::ColorSpaceKHR::eHdr10HlgEXT
      };

      // For 16-bit formats, prefer linear color spaces, then HDR10 color spaces, then other nonlinear color spaces
      static const std::array<vk::ColorSpaceKHR, 7> kPreferred16BitColorSpaces =
      {
         vk::ColorSpaceKHR::eBt2020LinearEXT,
         vk::ColorSpaceKHR::eDisplayP3LinearEXT,
         vk::ColorSpaceKHR::eExtendedSrgbLinearEXT,

         vk::ColorSpaceKHR::eHdr10St2084EXT,
         vk::ColorSpaceKHR::eHdr10HlgEXT,

         vk::ColorSpaceKHR::eDisplayP3NonlinearEXT,
         vk::ColorSpaceKHR::eExtendedSrgbNonlinearEXT
      };

      switch (format)
      {
      case vk::Format::eA2R10G10B10UnormPack32:
      case vk::Format::eA2B10G10R10UnormPack32:
         return kPreferred10BitColorSpaces;
      case vk::Format::eR16G16B16A16Sfloat:
         return kPreferred16BitColorSpaces;
      default:
         return kPreferred8BitColorSpaces;
      }
   }

   std::optional<vk::SurfaceFormatKHR> chooseSurfaceFormatForFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats, vk::Format format)
   {
      std::vector<vk::ColorSpaceKHR> availableColorSpaces = getColorSpacesForFormat(surfaceFormats, format);
      std::span<const vk::ColorSpaceKHR> preferredColorSpaces = getPreferredColorSpacesForFormat(format);

      std::erase_if(availableColorSpaces, [preferredColorSpaces](vk::ColorSpaceKHR colorSpace)
      {
         return !std::ranges::contains(preferredColorSpaces, colorSpace);
      });

      if (availableColorSpaces.empty())
      {
         return std::nullopt;
      }

      std::ranges::stable_sort(availableColorSpaces, [preferredColorSpaces](vk::ColorSpaceKHR a, vk::ColorSpaceKHR b)
      {
         auto aLocation = std::ranges::find(preferredColorSpaces, a);
         auto bLocation = std::ranges::find(preferredColorSpaces, b);

         return aLocation < bLocation;
      });

      return vk::SurfaceFormatKHR(format, availableColorSpaces[0]);
   }

   vk::SurfaceFormatKHR chooseDefaultSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats, const SwapchainSettings& settings)
   {
      if (settings.preferHDR)
      {
         if (std::optional<vk::SurfaceFormatKHR> abgr10SurfaceFormat = chooseSurfaceFormatForFormat(surfaceFormats, vk::Format::eA2B10G10R10UnormPack32))
         {
            ASSERT(SurfaceFormatHelpers::isHdrSurfaceFormat(abgr10SurfaceFormat.value()));
            return abgr10SurfaceFormat.value();
         }

         if (std::optional<vk::SurfaceFormatKHR> argb10SurfaceFormat = chooseSurfaceFormatForFormat(surfaceFormats, vk::Format::eA2R10G10B10UnormPack32))
         {
            ASSERT(SurfaceFormatHelpers::isHdrSurfaceFormat(argb10SurfaceFormat.value()));
            return argb10SurfaceFormat.value();
         }

         if (std::optional<vk::SurfaceFormatKHR> rgba16SurfaceFormat = chooseSurfaceFormatForFormat(surfaceFormats, vk::Format::eR16G16B16A16Sfloat))
         {
            ASSERT(SurfaceFormatHelpers::isHdrSurfaceFormat(rgba16SurfaceFormat.value()));
            return rgba16SurfaceFormat.value();
         }
      }

      if (std::optional<vk::SurfaceFormatKHR> bgra8SurfaceFormat = chooseSurfaceFormatForFormat(surfaceFormats, vk::Format::eB8G8R8A8Srgb))
      {
         return bgra8SurfaceFormat.value();
      }

      if (!surfaceFormats.empty())
      {
         return surfaceFormats[0];
      }

      throw std::runtime_error("No swapchain formats available");
   }

   vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats, const SwapchainSettings& settings)
   {
      vk::SurfaceFormatKHR surfaceFormat = chooseDefaultSurfaceFormat(surfaceFormats, settings);

      if (settings.desiredFormat.has_value())
      {
         if (std::optional<vk::SurfaceFormatKHR> desiredSurfaceFormat = chooseSurfaceFormatForFormat(surfaceFormats, settings.desiredFormat.value()))
         {
            surfaceFormat = desiredSurfaceFormat.value();
         }
      }

      if (settings.desiredColorSpace.has_value())
      {
         vk::SurfaceFormatKHR desiredSurfaceFormat(surfaceFormat.format, settings.desiredColorSpace.value());
         if (std::ranges::contains(surfaceFormats, desiredSurfaceFormat))
         {
            surfaceFormat = desiredSurfaceFormat;
         }
      }

      return surfaceFormat;
   }

   vk::PresentModeKHR chooseDefaultPresentMode(const std::vector<vk::PresentModeKHR>& presentModes, const SwapchainSettings& settings)
   {
      if (!settings.limitFrameRate)
      {
         if (std::ranges::contains(presentModes, vk::PresentModeKHR::eMailbox))
         {
            return vk::PresentModeKHR::eMailbox;
         }

         if (std::ranges::contains(presentModes, vk::PresentModeKHR::eImmediate))
         {
            return vk::PresentModeKHR::eImmediate;
         }
      }

      if (std::ranges::contains(presentModes, vk::PresentModeKHR::eFifo))
      {
         return vk::PresentModeKHR::eFifo;
      }

      if (!presentModes.empty())
      {
         return presentModes[0];
      }

      throw std::runtime_error("No swapchain present modes available");
   }

   vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes, const SwapchainSettings& settings)
   {
      if (settings.desiredPresentMode.has_value() && std::ranges::contains(presentModes, settings.desiredPresentMode.value()))
      {
         return settings.desiredPresentMode.value();
      }

      return chooseDefaultPresentMode(presentModes, settings);
   }
}

Swapchain::Swapchain(const GraphicsContext& graphicsContext, vk::Extent2D desiredExtent, const SwapchainSettings& settings)
   : GraphicsResource(graphicsContext)
   , capabilities(graphicsContext.determineSwapchainCapabilities())
{
   ASSERT(!capabilities.presentModes.empty() && !capabilities.surfaceFormats.empty());

   presentMode = choosePresentMode(capabilities.presentModes, settings);

   surfaceFormat = chooseSurfaceFormat(capabilities.surfaceFormats, settings);
   LOG_INFO("Swapchain surface format = " << vk::to_string(surfaceFormat.format) << ", color space = " << vk::to_string(surfaceFormat.colorSpace));

   vk::SurfaceCapabilitiesKHR surfaceCapabilities = context.getPhysicalDevice().getSurfaceCapabilitiesKHR(context.getSurface());

   extent = chooseExtent(surfaceCapabilities, desiredExtent);

   uint32_t desiredMinImageCount = surfaceCapabilities.minImageCount + (presentMode == vk::PresentModeKHR::eFifo ? 0 : 1);
   minImageCount = surfaceCapabilities.maxImageCount > 0 ? std::min(surfaceCapabilities.maxImageCount, desiredMinImageCount) : desiredMinImageCount;

   vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
      .setSurface(context.getSurface())
      .setMinImageCount(minImageCount)
      .setImageFormat(surfaceFormat.format)
      .setImageColorSpace(surfaceFormat.colorSpace)
      .setImageExtent(extent)
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPreTransform(surfaceCapabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(presentMode)
      .setClipped(true)
      .setImageSharingMode(vk::SharingMode::eExclusive);

   std::array<uint32_t, 2> indices = { context.getGraphicsFamilyIndex(), context.getPresentFamilyIndex() };
   if (context.getGraphicsFamilyIndex() != context.getPresentFamilyIndex())
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

bool Swapchain::supportsHDR() const
{
   for (const vk::SurfaceFormatKHR& surfaceFormat : capabilities.surfaceFormats)
   {
      if (SurfaceFormatHelpers::isHdrSurfaceFormat(surfaceFormat))
      {
         return true;
      }
   }

   return false;
}

Texture& Swapchain::getCurrentTexture() const
{
   uint32_t index = context.getSwapchainIndex();
   ASSERT(index < textures.size() && textures[index] != nullptr);

   return *textures[index];
}
