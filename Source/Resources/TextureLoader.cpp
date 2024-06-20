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
#include <vector>

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

   struct Color
   {
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
      uint8_t a = 0;

      static uint8_t quantize(float value)
      {
         return static_cast<uint8_t>(glm::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
      }

      Color() = default;

      Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
         : r(red)
         , g(green)
         , b(blue)
         , a(alpha)
      {
      }

      Color(const glm::vec4& color)
         : r(quantize(color.r))
         , g(quantize(color.g))
         , b(quantize(color.b))
         , a(quantize(color.a))
      {
      }
   };

   class DefaultImage : public Image
   {
   public:
      DefaultImage(const Color& color, uint32_t depth = 1)
      {
         properties.format = vk::Format::eR8G8B8A8Unorm;
         properties.type = depth == 1 ? vk::ImageType::e2D : vk::ImageType::e3D;
         properties.width = 2;
         properties.height = 2;
         properties.depth = depth;

         data.assign(properties.width * properties.height * properties.depth * properties.layers, color);

         MipInfo mipInfo;
         mipInfo.extent = vk::Extent3D(properties.width, properties.height, properties.depth);
         mips.push_back(mipInfo);
      }

      DefaultImage(const std::array<Color, 6>& faceColors)
      {
         properties.format = vk::Format::eR8G8B8A8Unorm;
         properties.width = 2;
         properties.height = 2;
         properties.layers = faceColors.size();
         properties.cubeCompatible = true;

         uint32_t numValuesPerLayer = properties.width * properties.height * properties.depth;
         data.resize(numValuesPerLayer * properties.layers);

         MipInfo mipInfo;
         mipInfo.extent = vk::Extent3D(properties.width, properties.height, properties.depth);

         for (std::size_t layer = 0; layer < properties.layers; ++layer)
         {
            for (std::size_t i = 0; i < numValuesPerLayer; ++i)
            {
               data[layer * numValuesPerLayer + i] = faceColors[layer];
            }

            mipInfo.bufferOffset = layer * (numValuesPerLayer * sizeof(Color));
            mips.push_back(mipInfo);
         }
      }

      TextureData getTextureData() const final
      {
         TextureData textureData;
         textureData.bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(Color));
         textureData.mips = mips;
         textureData.mipsPerLayer = 1;
         return textureData;
      }

   private:
      std::vector<Color> data;
      std::vector<MipInfo> mips;
   };

   DefaultImage createDefaultImage(DefaultTextureType type)
   {
      switch (type)
      {
      case DefaultTextureType::Black:
         return DefaultImage(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
      case DefaultTextureType::White:
         return DefaultImage(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
      case DefaultTextureType::Normal:
         return DefaultImage(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
      case DefaultTextureType::AoRoughnessMetalness:
         return DefaultImage(glm::vec4(0.0f, 0.75f, 0.0f, 1.0f));
      case DefaultTextureType::Cube:
         return DefaultImage(std::array<Color, 6>
         {
            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
         });
      case DefaultTextureType::Volume:
         return DefaultImage(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 2);
      default:
         ASSERT(false);
         return DefaultImage(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
      }
   }

   const std::string& getDefaultName(DefaultTextureType type)
   {
      static const std::string kNoneName = "";
      static const std::string kBlackName = "Default Black Texture";
      static const std::string kWhiteName = "Default White Texture";
      static const std::string kNormalName = "Default Normal Texture";
      static const std::string kAoRoughnessMetalnessName = "Default AO Roughness Metalness Texture";
      static const std::string kCubeName = "Default Cube Texture";
      static const std::string kVolumeName = "Default Volume Texture";

      switch (type)
      {
      case DefaultTextureType::None:
         ASSERT(false);
         return kNoneName;
      case DefaultTextureType::Black:
         return kBlackName;
      case DefaultTextureType::White:
         return kWhiteName;
      case DefaultTextureType::Normal:
         return kNormalName;
      case DefaultTextureType::AoRoughnessMetalness:
         return kAoRoughnessMetalnessName;
      case DefaultTextureType::Cube:
         return kCubeName;
      case DefaultTextureType::Volume:
         return kVolumeName;
      default:
         ASSERT(false);
         return kNoneName;
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
   , defaultBlack(createDefault(DefaultTextureType::Black))
   , defaultWhite(createDefault(DefaultTextureType::White))
   , defaultNormal(createDefault(DefaultTextureType::Normal))
   , defaultAoRoughnessMetalness(createDefault(DefaultTextureType::AoRoughnessMetalness))
   , defaultCube(createDefault(DefaultTextureType::Cube))
   , defaultVolume(createDefault(DefaultTextureType::Volume))
{
}

TextureHandle TextureLoader::load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions)
{
   TextureKey key;
   key.options = loadOptions;

   if (std::optional<std::filesystem::path> canonicalPath = ResourceLoadHelpers::makeCanonical(path))
   {
      key.canonicalPath = canonicalPath->string();

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
   else
   {
      key.canonicalPath = path.string();
   }

   if (Texture* defaultTexture = getDefault(loadOptions.fallbackDefaultTextureType))
   {
      return container.addReference(key, defaultTexture);
   }

   return TextureHandle{};
}

Texture* TextureLoader::getDefault(DefaultTextureType type)
{
   switch (type)
   {
   case DefaultTextureType::None:
      return nullptr;
   case DefaultTextureType::Black:
      return defaultBlack.get();
   case DefaultTextureType::White:
      return defaultWhite.get();
   case DefaultTextureType::Normal:
      return defaultNormal.get();
   case DefaultTextureType::AoRoughnessMetalness:
      return defaultAoRoughnessMetalness.get();
   case DefaultTextureType::Cube:
      return defaultCube.get();
   case DefaultTextureType::Volume:
      return defaultVolume.get();
   default:
      ASSERT(false);
      return nullptr;
   }
}

const Texture* TextureLoader::getDefault(DefaultTextureType type) const
{
   switch (type)
   {
   case DefaultTextureType::None:
      return nullptr;
   case DefaultTextureType::Black:
      return defaultBlack.get();
   case DefaultTextureType::White:
      return defaultWhite.get();
   case DefaultTextureType::Normal:
      return defaultNormal.get();
   case DefaultTextureType::AoRoughnessMetalness:
      return defaultAoRoughnessMetalness.get();
   case DefaultTextureType::Cube:
      return defaultCube.get();
   case DefaultTextureType::Volume:
      return defaultVolume.get();
   default:
      ASSERT(false);
      return nullptr;
   }
}

std::unique_ptr<Texture> TextureLoader::createDefault(DefaultTextureType type) const
{
   ASSERT(type != DefaultTextureType::None);

   DefaultImage defaultImage = createDefaultImage(type);
   std::unique_ptr<Texture> defaultTexture = std::make_unique<Texture>(context, defaultImage.getProperties(), getTextureProperties(false), getInitialLayout(), defaultImage.getTextureData());
   NAME_POINTER(context.getDevice(), defaultTexture.get(), getDefaultName(type));

   return defaultTexture;
}
