#pragma once

#include "Graphics/Context.h"

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

class Texture
{
public:
   // Create a texture with no initial data
   Texture(const VulkanContext& context, const ImageProperties& imageProps, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout);

   // Create a texture with initial data provided by a loaded image
   Texture(const VulkanContext& context, const LoadedImage& loadedImage, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout);

   Texture(const Texture& other) = delete;
   Texture(Texture&& other);

   ~Texture();

   Texture& operator=(const Texture& other) = delete;
   Texture& operator=(Texture&& other);

private:
   void move(Texture&& other);
   void release();

public:
   vk::ImageView createView(const VulkanContext& context, vk::ImageViewType viewType) const;
   void transitionLayout(const VulkanContext& context, vk::ImageLayout newLayout, const TextureMemoryBarrierFlags& srcMemoryBarrierFlags, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags);

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
   void createImage(const VulkanContext& context);
   void copyBufferToImage(const VulkanContext& context, vk::Buffer buffer);
   void stageAndCopyImage(const VulkanContext& context, const LoadedImage& loadedImage);
   void generateMipmaps(const VulkanContext& context, vk::ImageLayout finalLayout, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags);

   vk::Device device;

   vk::Image image;
   vk::DeviceMemory memory;
   vk::ImageView defaultView;

   ImageProperties imageProperties;
   TextureProperties textureProperties;

   vk::ImageLayout layout = vk::ImageLayout::eUndefined;
   uint32_t mipLevels = 1;
};