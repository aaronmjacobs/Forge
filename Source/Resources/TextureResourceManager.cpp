#include "Resources/TextureResourceManager.h"

#include "Resources/LoadedImage.h"

#include <PlatformUtils/IOUtils.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
   std::optional<LoadedImage> loadImage(const std::filesystem::path& path, const TextureLoadOptions& loadOptions)
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
            loadedImage.size = static_cast<vk::DeviceSize>(textureWidth * textureHeight * STBI_rgb_alpha);

            loadedImage.properties.format = loadOptions.sRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
            loadedImage.properties.width = static_cast<uint32_t>(textureWidth);
            loadedImage.properties.height = static_cast<uint32_t>(textureHeight);

            return loadedImage;
         }
      }

      return {};
   }
}

TextureResourceManager::TextureResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceManagerBase(graphicsContext, owningResourceManager)
{
}

TextureHandle TextureResourceManager::load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions, const TextureProperties& properties, const TextureInitialLayout& initialLayout)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceHelpers::makeCanonical(path))
   {
      std::string canonicalPathString = canonicalPath->string();
      if (std::optional<Handle> cachedHandle = getCachedHandle(canonicalPathString))
      {
         return *cachedHandle;
      }

      if (std::optional<LoadedImage> image = loadImage(*canonicalPath, loadOptions))
      {
         TextureHandle handle = emplaceResource(context, *image, properties, initialLayout);
         cacheHandle(canonicalPathString, handle);

         return handle;
      }
   }

   return TextureHandle();
}

// static
TextureProperties TextureResourceManager::getDefaultProperties()
{
   TextureProperties defaultProperties;

   defaultProperties.generateMipMaps = true;

   return defaultProperties;
}

// static
TextureInitialLayout TextureResourceManager::getDefaultInitialLayout()
{
   TextureInitialLayout defaultInitialLayout;

   defaultInitialLayout.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
   defaultInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);

   return defaultInitialLayout;
}
