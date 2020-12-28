#pragma once

#include "Graphics/GraphicsResource.h"

#include <vector>

struct LoadedImage;

struct ImageProperties
{
   vk::Format format = vk::Format::eR8G8B8A8Srgb;
   vk::ImageType type = vk::ImageType::e2D;
   uint32_t width = 1;
   uint32_t height = 1;
   uint32_t depth = 1;
};

struct TextureProperties
{
   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
   vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
   vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
   vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
   vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;

   bool generateMipMaps = false;
};

struct TextureMemoryBarrierFlags
{
   vk::AccessFlags accessMask;
   vk::PipelineStageFlags stageMask;

   TextureMemoryBarrierFlags(vk::AccessFlags accessFlags = vk::AccessFlags(), vk::PipelineStageFlags stageFlags = vk::PipelineStageFlags())
      : accessMask(accessFlags)
      , stageMask(stageFlags)
   {
   }
};

struct TextureInitialLayout
{
   vk::ImageLayout layout = vk::ImageLayout::eUndefined;
   TextureMemoryBarrierFlags memoryBarrierFlags;
};

class Texture : public GraphicsResource
{
public:
   static vk::Format findSupportedFormat(const GraphicsContext& context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

   // Create a texture with no initial data
   Texture(const GraphicsContext& graphicsContext, const ImageProperties& imageProps, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout);

   // Create a texture with initial data provided by a loaded image
   Texture(const GraphicsContext& graphicsContext, const LoadedImage& loadedImage, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout);

   ~Texture();

   vk::ImageView createView(vk::ImageViewType viewType) const;
   void transitionLayout(vk::ImageLayout newLayout, const TextureMemoryBarrierFlags& srcMemoryBarrierFlags, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags);

   vk::ImageView getDefaultView() const
   {
      return defaultView;
   }

   const ImageProperties& getImageProperties() const
   {
      return imageProperties;
   }

   const TextureProperties& getTextureProperties() const
   {
      return textureProperties;
   }

   vk::ImageLayout getLayout() const
   {
      return layout;
   }

   uint32_t getMipLevels() const
   {
      return mipLevels;
   }

private:
   void createImage();
   void copyBufferToImage(vk::Buffer buffer);
   void stageAndCopyImage(const LoadedImage& loadedImage);
   void generateMipmaps(vk::ImageLayout finalLayout, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags);

   vk::Image image;
   vk::DeviceMemory memory;
   vk::ImageView defaultView;

   ImageProperties imageProperties;
   TextureProperties textureProperties;

   vk::ImageLayout layout = vk::ImageLayout::eUndefined;
   uint32_t mipLevels = 1;
};
