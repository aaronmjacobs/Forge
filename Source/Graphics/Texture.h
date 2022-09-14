#pragma once

#include "Core/Hash.h"

#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"

#include <optional>
#include <span>
#include <unordered_map>

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

struct ImageViewDesc
{
   vk::ImageViewType viewType = vk::ImageViewType::e2D;
   uint32_t baseLayer = 0;
   uint32_t layerCount = 1;
   vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eNone;

   bool operator==(const ImageViewDesc& other) const = default;

   std::size_t hash() const
   {
      std::size_t hash = 0;

      Hash::combine(hash, Hash::of(viewType));
      Hash::combine(hash, Hash::of(baseLayer));
      Hash::combine(hash, Hash::of(layerCount));
      Hash::combine(hash, Hash::of(static_cast<VkImageAspectFlags>(aspectFlags)));

      return hash;
   }
};

USE_MEMBER_HASH_FUNCTION(ImageViewDesc);

enum class TextureLayoutType
{
   AttachmentWrite,
   ShaderRead,
   Present
};

class Texture : public GraphicsResource
{
public:
   static vk::Format findSupportedFormat(const GraphicsContext& context, std::span<const vk::Format> candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

   Texture(const GraphicsContext& graphicsContext, const ImageProperties& imageProps, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout, const TextureData& textureData = {});
   Texture(const GraphicsContext& graphicsContext, const ImageProperties& imageProps, vk::Image swapchainImage);
   ~Texture();

   vk::ImageView getOrCreateView(vk::ImageViewType viewType, uint32_t baseLayer = 0, uint32_t layerCount = 1, std::optional<vk::ImageAspectFlags> aspectFlags = {}, bool* created = nullptr);
   void transitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout, const TextureMemoryBarrierFlags& srcMemoryBarrierFlags, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags);
   void transitionLayout(vk::CommandBuffer commandBuffer, TextureLayoutType layoutType);

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

   vk::Extent2D getExtent() const
   {
      return vk::Extent2D(imageProperties.width, imageProperties.height);
   }

private:
   void createImage();
   void createDefaultView();
   void copyBufferToImage(vk::Buffer buffer, const TextureData& textureData);
   void stageAndCopyImage(const TextureData& textureData);
   void generateMipmaps(vk::ImageLayout finalLayout, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags);

   vk::Image image;
   VmaAllocation imageAllocation = nullptr;
   vk::ImageView defaultView;

   ImageProperties imageProperties;
   TextureProperties textureProperties;

   vk::ImageLayout layout = vk::ImageLayout::eUndefined;
   uint32_t mipLevels = 1;

   std::unordered_map<ImageViewDesc, vk::ImageView> viewMap;
};
