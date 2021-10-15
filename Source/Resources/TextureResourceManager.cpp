#include "Resources/TextureResourceManager.h"

#include "Graphics/DebugUtils.h"

#include "Resources/LoadedImage.h"

#include <PlatformUtils/IOUtils.h>

#include <glm/glm.hpp>

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
            loadedImage.properties.hasAlpha = textureChannels == 4;

            return loadedImage;
         }
      }

      return {};
   }

   LoadedImage createDefaultImage(const GraphicsContext& context, const glm::vec4& color)
   {
      static const auto to8Bit = [](float value)
      {
         return static_cast<uint8_t>(glm::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
      };

      LoadedImage loadedImage;

      uint8_t* pixelData = static_cast<uint8_t*>(STBI_MALLOC(4));
      pixelData[0] = to8Bit(color.r);
      pixelData[1] = to8Bit(color.g);
      pixelData[2] = to8Bit(color.b);
      pixelData[3] = to8Bit(color.a);
      loadedImage.data.reset(pixelData);
      loadedImage.size = static_cast<vk::DeviceSize>(4);

      loadedImage.properties.format = vk::Format::eR8G8B8A8Unorm;
      loadedImage.properties.width = 1;
      loadedImage.properties.height = 1;

      return loadedImage;
   }
}

TextureResourceManager::TextureResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceManagerBase(graphicsContext, owningResourceManager)
{
   createDefaultTextures();
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

         NAME_POINTER(context.getDevice(), get(handle), ResourceHelpers::getName(*canonicalPath));

         return handle;
      }
   }

   return getDefault(loadOptions.fallbackDefaultTextureType);
}

TextureHandle TextureResourceManager::getDefault(DefaultTextureType type) const
{
   TextureHandle handle;
   switch (type)
   {
   case DefaultTextureType::None:
      break;
   case DefaultTextureType::Black:
      handle = defaultBlackTextureHandle;
      break;
   case DefaultTextureType::White:
      handle = defaultWhiteTextureHandle;
      break;
   case DefaultTextureType::NormalMap:
      handle = defaultNormalMapTextureHandle;
      break;
   default:
      break;
   }

   return handle;
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

void TextureResourceManager::createDefaultTextures()
{
   LoadedImage defaultBlackImage = createDefaultImage(context, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
   LoadedImage defaultWhiteImage = createDefaultImage(context, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
   LoadedImage defaultNormalMapImage = createDefaultImage(context, glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));

   TextureProperties defaultTextureProperties = getDefaultProperties();
   defaultTextureProperties.generateMipMaps = false;

   TextureInitialLayout defaultTextureInitialLayout = getDefaultInitialLayout();

   defaultBlackTextureHandle = emplaceResource(context, defaultBlackImage, defaultTextureProperties, defaultTextureInitialLayout);
   NAME_POINTER(context.getDevice(), get(defaultBlackTextureHandle), "Default Black Texture");

   defaultWhiteTextureHandle = emplaceResource(context, defaultWhiteImage, defaultTextureProperties, defaultTextureInitialLayout);
   NAME_POINTER(context.getDevice(), get(defaultWhiteTextureHandle), "Default White Texture");

   defaultNormalMapTextureHandle = emplaceResource(context, defaultNormalMapImage, defaultTextureProperties, defaultTextureInitialLayout);
   NAME_POINTER(context.getDevice(), get(defaultNormalMapTextureHandle), "Default Normal Map Texture");
}
