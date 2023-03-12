#include "Resources/STBImage.h"

#include "Resources/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
   class STBImage : public Image
   {
   public:
      STBImage(const ImageProperties& imageProperties, stbi_uc* pixelData)
         : Image(imageProperties)
         , data(pixelData)
      {
         mipInfo.extent = vk::Extent3D(imageProperties.width, imageProperties.height, imageProperties.depth);
      }

      ~STBImage()
      {
         STBI_FREE(data);
      }

      TextureData getTextureData() const final
      {
         TextureData textureData;
         textureData.bytes = std::span<const uint8_t>(data, properties.width * properties.height * STBI_rgb_alpha);
         textureData.mips = std::span<const MipInfo>(&mipInfo, 1);
         textureData.mipsPerLayer = 1;
         return textureData;
      }

   private:
      stbi_uc* data = nullptr;
      MipInfo mipInfo;
   };
}

namespace STB
{
   std::unique_ptr<Image> loadImage(std::vector<uint8_t> fileData, bool sRGB)
   {
      if (fileData.size() > std::numeric_limits<int>::max())
      {
         return nullptr;
      }

      int textureWidth = 0;
      int textureHeight = 0;
      int textureChannels = 0;
      stbi_uc* pixelData = stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
      if (!pixelData)
      {
         return nullptr;
      }

      ImageProperties properties;
      properties.format = sRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
      properties.width = static_cast<uint32_t>(textureWidth);
      properties.height = static_cast<uint32_t>(textureHeight);
      properties.hasAlpha = textureChannels == 4;

      return std::make_unique<STBImage>(properties, pixelData);
   }
}
