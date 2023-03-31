#include "Resources/TextureLoader.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"

#include "Resources/DDSImage.h"
#include "Resources/Image.h"
#include "Resources/STBImage.h"

#include <PlatformUtils/IOUtils.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

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
            return DDS::loadImage(std::move(*fileData), loadOptions.sRGB);
         }
         else
         {
            return STB::loadImage(std::move(*fileData), loadOptions.sRGB);
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

   std::optional<SinglePixelImage> createDefaultImage(DefaultTextureType type)
   {
      switch (type)
      {
      case DefaultTextureType::Black:
         return createDefaultImage(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
      case DefaultTextureType::White:
         return createDefaultImage(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
      case DefaultTextureType::NormalMap:
         return createDefaultImage(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
      case DefaultTextureType::AoRoughnessMetalnessMap:
         return createDefaultImage(glm::vec4(0.0f, 0.75f, 0.0f, 1.0f));
      default:
         return {};
      }
   }

   TextureProperties getTextureProperties(bool generateMipMaps)
   {
      TextureProperties defaultProperties;

      defaultProperties.generateMipMaps = generateMipMaps;

      return defaultProperties;
   }

   TextureInitialLayout getInitialLayout()
   {
      TextureInitialLayout defaultInitialLayout;

      defaultInitialLayout.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
      defaultInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);

      return defaultInitialLayout;
   }
}

std::size_t TextureKey::hash() const
{
   std::size_t hash = 0;

   Hash::combine(hash, canonicalPath);

   Hash::combine(hash, options.sRGB);
   Hash::combine(hash, options.generateMipMaps);

   return hash;
}

TextureLoader::TextureLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceLoader(graphicsContext, owningResourceManager)
{
}

TextureHandle TextureLoader::load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceLoadHelpers::makeCanonical(path))
   {
      TextureKey key;
      key.canonicalPath = canonicalPath->string();
      key.options = loadOptions;

      if (key.canonicalPath.starts_with('*'))
      {
         // Reserved for defaults
         return TextureHandle{};
      }

      if (Handle cachedHandle = container.findHandle(key))
      {
         return cachedHandle;
      }

      if (std::unique_ptr<Image> image = loadImage(*canonicalPath, loadOptions))
      {
         TextureHandle handle = container.emplace(key, context, image->getProperties(), getTextureProperties(loadOptions.generateMipMaps), getInitialLayout(), image->getTextureData());
         NAME_POINTER(context.getDevice(), get(handle), ResourceLoadHelpers::getName(*canonicalPath));

         return handle;
      }
   }

   return TextureHandle{};
}

TextureHandle TextureLoader::createDefault(DefaultTextureType type)
{
   if (type == DefaultTextureType::None)
   {
      return TextureHandle{};
   }

   TextureKey key;
   key.canonicalPath = getDefaultPath(type);
   key.options.sRGB = false;
   key.options.generateMipMaps = false;

   if (Handle cachedHandle = container.findHandle(key))
   {
      return cachedHandle;
   }

   std::optional<SinglePixelImage> defaultImage = createDefaultImage(type);
   if (defaultImage.has_value())
   {
      TextureHandle handle = container.emplace(key, context, defaultImage->getProperties(), getTextureProperties(false), getInitialLayout(), defaultImage->getTextureData());
      NAME_POINTER(context.getDevice(), get(handle), getDefaultName(type));

      return handle;
   }

   return TextureHandle{};
}

const std::string& TextureLoader::getDefaultPath(DefaultTextureType type) const
{
   static const std::string kNonePath = "*none*";
   static const std::string kBlackPath = "*default|black*";
   static const std::string kWhitePath = "*default|white*";
   static const std::string kNormalPath = "*default|normal*";
   static const std::string kAoRoughnessMetalnessPath = "*default|ao_roughness_metalness*";

   switch (type)
   {
   case DefaultTextureType::None:
      ASSERT(false);
      return kNonePath;
   case DefaultTextureType::Black:
      return kBlackPath;
   case DefaultTextureType::White:
      return kWhitePath;
   case DefaultTextureType::NormalMap:
      return kNormalPath;
   case DefaultTextureType::AoRoughnessMetalnessMap:
      return kAoRoughnessMetalnessPath;
   default:
      ASSERT(false);
      return kNonePath;
   }
}

const std::string& TextureLoader::getDefaultName(DefaultTextureType type) const
{
   static const std::string kNoneName = "";
   static const std::string kBlackName = "Default Black Texture";
   static const std::string kWhiteName = "Default White Texture";
   static const std::string kNormalName = "Default Normal Map Texture";
   static const std::string kAoRoughnessMetalnessName = "Default AO Roughness Metalness Map Texture";

   switch (type)
   {
   case DefaultTextureType::None:
      ASSERT(false);
      return kNoneName;
   case DefaultTextureType::Black:
      return kBlackName;
   case DefaultTextureType::White:
      return kWhiteName;
   case DefaultTextureType::NormalMap:
      return kNormalName;
   case DefaultTextureType::AoRoughnessMetalnessMap:
      return kAoRoughnessMetalnessName;
   default:
      ASSERT(false);
      return kNoneName;
   }
}
