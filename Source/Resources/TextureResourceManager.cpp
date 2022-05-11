#include "Resources/TextureResourceManager.h"

#include "Graphics/DebugUtils.h"

#include "Resources/DDSImageLoader.h"
#include "Resources/Image.h"
#include "Resources/STBImageLoader.h"

#include <PlatformUtils/IOUtils.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cstdint>

namespace
{
   std::unique_ptr<Image> loadImage(const std::filesystem::path& path, const TextureLoadOptions& loadOptions)
   {
      if (std::optional<std::vector<uint8_t>> fileData = IOUtils::readBinaryFile(path))
      {
         std::string extension = path.extension().string();
         std::transform(extension.begin(), extension.end(), extension.begin(), [](const char c) { return std::tolower(c); });

         if (extension == ".dds")
         {
            return DDSImageLoader::loadImage(std::move(*fileData), loadOptions.sRGB);
         }
         else
         {
            return STBImageLoader::loadImage(std::move(*fileData), loadOptions.sRGB);
         }
      }

      return nullptr;
   }

   class SinglePixelImage : public Image
   {
   public:
      SinglePixelImage(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
         : data({ r, g, b, a })
      {
         properties.format = vk::Format::eR8G8B8A8Unorm;
         properties.width = 1;
         properties.height = 1;

         mipInfo.extent = vk::Extent3D(1, 1, 1);
      }

      TextureData getTextureData() const final
      {
         TextureData textureData;
         textureData.bytes = std::span<const uint8_t>(data);
         textureData.mips = std::span<const MipInfo>(&mipInfo, 1);
         textureData.mipsPerLayer = 1;
         return textureData;
      }

   private:
      std::array<uint8_t, 4> data;
      MipInfo mipInfo;
   };

   SinglePixelImage createDefaultImage(const glm::vec4& color)
   {
      static const auto to8Bit = [](float value)
      {
         return static_cast<uint8_t>(glm::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
      };

      return SinglePixelImage(to8Bit(color.r), to8Bit(color.g), to8Bit(color.b), to8Bit(color.a));
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

      if (std::unique_ptr<Image> image = loadImage(*canonicalPath, loadOptions))
      {
         TextureHandle handle = emplaceResource(context, image->getProperties(), properties, initialLayout, image->getTextureData());
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
   case DefaultTextureType::AoRoughnessMetalnessMap:
      handle = defaultAoRoughnessMetalnessMapTextureHandle;
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

void TextureResourceManager::onAllResourcesUnloaded()
{
   createDefaultTextures();
}

void TextureResourceManager::createDefaultTextures()
{
   SinglePixelImage defaultBlackImage = createDefaultImage(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
   SinglePixelImage defaultWhiteImage = createDefaultImage(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
   SinglePixelImage defaultNormalMapImage = createDefaultImage(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
   SinglePixelImage defaultAoRoughnessMetalnessMapImage = createDefaultImage(glm::vec4(0.0f, 0.75f, 0.0f, 1.0f));

   TextureProperties defaultTextureProperties = getDefaultProperties();
   defaultTextureProperties.generateMipMaps = false;

   TextureInitialLayout defaultTextureInitialLayout = getDefaultInitialLayout();

   defaultBlackTextureHandle = emplaceResource(context, defaultBlackImage.getProperties(), defaultTextureProperties, defaultTextureInitialLayout, defaultBlackImage.getTextureData());
   NAME_POINTER(context.getDevice(), get(defaultBlackTextureHandle), "Default Black Texture");

   defaultWhiteTextureHandle = emplaceResource(context, defaultWhiteImage.getProperties(), defaultTextureProperties, defaultTextureInitialLayout, defaultWhiteImage.getTextureData());
   NAME_POINTER(context.getDevice(), get(defaultWhiteTextureHandle), "Default White Texture");

   defaultNormalMapTextureHandle = emplaceResource(context, defaultNormalMapImage.getProperties(), defaultTextureProperties, defaultTextureInitialLayout, defaultNormalMapImage.getTextureData());
   NAME_POINTER(context.getDevice(), get(defaultNormalMapTextureHandle), "Default Normal Map Texture");

   defaultAoRoughnessMetalnessMapTextureHandle = emplaceResource(context, defaultAoRoughnessMetalnessMapImage.getProperties(), defaultTextureProperties, defaultTextureInitialLayout, defaultAoRoughnessMetalnessMapImage.getTextureData());
   NAME_POINTER(context.getDevice(), get(defaultAoRoughnessMetalnessMapTextureHandle), "Default AO Roughness Metalness Map Texture");
}
