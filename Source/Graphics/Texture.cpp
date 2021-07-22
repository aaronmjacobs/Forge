#include "Graphics/Texture.h"

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/Command.h"
#include "Graphics/DebugUtils.h"
#include "Graphics/Memory.h"

#include "Resources/LoadedImage.h"

#include <cmath>
#include <utility>

namespace
{
   vk::ImageViewType getDefaultViewType(const ImageProperties& imageProperties)
   {
      switch (imageProperties.type)
      {
      case vk::ImageType::e1D:
         return vk::ImageViewType::e1D;
      case vk::ImageType::e2D:
         return vk::ImageViewType::e2D;
      case vk::ImageType::e3D:
         return vk::ImageViewType::e3D;
      default:
         ASSERT(false);
         return vk::ImageViewType::e2D;
      }
   }

   uint32_t calcMipLevels(const ImageProperties& imageProperties)
   {
      uint32_t maxDimension = std::max(imageProperties.width, std::max(imageProperties.height, imageProperties.depth));
      return static_cast<uint32_t>(std::log2(maxDimension)) + 1;
   }
}

// static
vk::Format Texture::findSupportedFormat(const GraphicsContext& context, std::span<const vk::Format> candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
   for (vk::Format format : candidates)
   {
      vk::FormatProperties properties = context.getPhysicalDevice().getFormatProperties(format);

      switch (tiling)
      {
      case vk::ImageTiling::eOptimal:
         if ((properties.optimalTilingFeatures & features) == features)
         {
            return format;
         }
         break;
      case vk::ImageTiling::eLinear:
         if ((properties.linearTilingFeatures & features) == features)
         {
            return format;
         }
         break;
      default:
         break;
      }
   }

   throw std::runtime_error("Failed to find supported format");
}

Texture::Texture(const GraphicsContext& graphicsContext, const ImageProperties& imageProps, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout)
   : GraphicsResource(graphicsContext)
   , imageProperties(imageProps)
   , textureProperties(textureProps)
{
   createImage();
   defaultView = createView(getDefaultViewType(imageProperties));

   transitionLayout(nullptr, initialLayout.layout, TextureMemoryBarrierFlags(vk::AccessFlags(), vk::PipelineStageFlagBits::eTopOfPipe), initialLayout.memoryBarrierFlags);
}

Texture::Texture(const GraphicsContext& graphicsContext, const LoadedImage& loadedImage, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout)
   : GraphicsResource(graphicsContext)
   , imageProperties(loadedImage.properties)
   , textureProperties(textureProps)
{
   textureProperties.usage |= vk::ImageUsageFlagBits::eTransferDst;
   if (textureProperties.generateMipMaps)
   {
      textureProperties.usage |= vk::ImageUsageFlagBits::eTransferSrc;
   }

   createImage();
   defaultView = createView(getDefaultViewType(imageProperties));

   stageAndCopyImage(loadedImage);

   if (textureProperties.generateMipMaps)
   {
      generateMipmaps(initialLayout.layout, initialLayout.memoryBarrierFlags);
   }
   else
   {
      transitionLayout(nullptr, initialLayout.layout, TextureMemoryBarrierFlags(vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer), initialLayout.memoryBarrierFlags);
   }
}

Texture::~Texture()
{
   ASSERT(defaultView);
   context.delayedDestroy(std::move(defaultView));

   ASSERT(image);
   context.delayedDestroy(std::move(image));

   ASSERT(memory);
   context.delayedFree(std::move(memory));
}

vk::ImageView Texture::createView(vk::ImageViewType viewType) const
{
   vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
      .setAspectMask(textureProperties.aspects)
      .setBaseMipLevel(0)
      .setLevelCount(mipLevels)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

   vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
      .setImage(image)
      .setViewType(viewType)
      .setFormat(imageProperties.format)
      .setSubresourceRange(subresourceRange);

   return device.createImageView(createInfo);
}

void Texture::transitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout, const TextureMemoryBarrierFlags& srcMemoryBarrierFlags, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags)
{
   if (layout != newLayout)
   {
      vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
         .setAspectMask(textureProperties.aspects)
         .setBaseMipLevel(0)
         .setLevelCount(mipLevels)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::ImageLayout oldLayout = layout;
      vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
         .setOldLayout(oldLayout)
         .setNewLayout(newLayout)
         .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
         .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
         .setImage(image)
         .setSubresourceRange(subresourceRange)
         .setSrcAccessMask(srcMemoryBarrierFlags.accessMask)
         .setDstAccessMask(dstMemoryBarrierFlags.accessMask);

      bool singleCommandBuffer = false;
      if (!commandBuffer)
      {
         singleCommandBuffer = true;
         commandBuffer = Command::beginSingle(context);
      }

      commandBuffer.pipelineBarrier(srcMemoryBarrierFlags.stageMask, dstMemoryBarrierFlags.stageMask, vk::DependencyFlags(), nullptr, nullptr, { barrier });

      if (singleCommandBuffer)
      {
         Command::endSingle(context, commandBuffer);
      }

      layout = newLayout;
   }
}

TextureInfo Texture::getInfo() const
{
   TextureInfo info;

   info.format = imageProperties.format;
   info.extent.width = imageProperties.width;
   info.extent.height = imageProperties.height;
   info.sampleCount = textureProperties.sampleCount;
   info.view = defaultView;
   info.isSwapchainTexture = false;

   return info;
}

#if FORGE_DEBUG
void Texture::setName(std::string_view newName)
{
   GraphicsResource::setName(newName);

   NAME_OBJECT(image, name);
   NAME_OBJECT(memory, name + " Memory");
   NAME_OBJECT(defaultView, name + " Default View");
}
#endif // FORGE_DEBUG

void Texture::createImage()
{
   ASSERT(!image && !memory);

   mipLevels = textureProperties.generateMipMaps ? calcMipLevels(imageProperties) : 1;

   vk::ImageCreateInfo imageCreateinfo = vk::ImageCreateInfo()
      .setImageType(imageProperties.type)
      .setExtent(vk::Extent3D(imageProperties.width, imageProperties.height, imageProperties.depth))
      .setMipLevels(mipLevels)
      .setArrayLayers(1)
      .setFormat(imageProperties.format)
      .setTiling(textureProperties.tiling)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setUsage(textureProperties.usage)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setSamples(textureProperties.sampleCount);
   image = device.createImage(imageCreateinfo);

   vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(image);
   vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
      .setAllocationSize(memoryRequirements.size)
      .setMemoryTypeIndex(Memory::findType(context.getPhysicalDevice(), memoryRequirements.memoryTypeBits, textureProperties.memoryProperties));
   memory = device.allocateMemory(allocateInfo);
   device.bindImageMemory(image, memory, 0);
}

void Texture::copyBufferToImage(vk::Buffer buffer)
{
   vk::CommandBuffer commandBuffer = Command::beginSingle(context);

   vk::ImageSubresourceLayers imageSubresource = vk::ImageSubresourceLayers()
      .setAspectMask(textureProperties.aspects)
      .setMipLevel(0)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

   vk::BufferImageCopy region = vk::BufferImageCopy()
      .setBufferOffset(0)
      .setBufferRowLength(0)
      .setBufferImageHeight(0)
      .setImageSubresource(imageSubresource)
      .setImageOffset(vk::Offset3D(0, 0))
      .setImageExtent(vk::Extent3D(imageProperties.width, imageProperties.height, imageProperties.depth));

   layout = vk::ImageLayout::eTransferDstOptimal;
   commandBuffer.copyBufferToImage(buffer, image, layout, { region });

   Command::endSingle(context, commandBuffer);
}

void Texture::stageAndCopyImage(const LoadedImage& loadedImage)
{
   transitionLayout(nullptr, vk::ImageLayout::eTransferDstOptimal, TextureMemoryBarrierFlags(vk::AccessFlags(), vk::PipelineStageFlagBits::eTopOfPipe), TextureMemoryBarrierFlags(vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer));

   vk::Buffer stagingBuffer;
   vk::DeviceMemory stagingBufferMemory;
   Buffer::create(context, loadedImage.size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

   void* mappedMemory = device.mapMemory(stagingBufferMemory, 0, loadedImage.size);
   std::memcpy(mappedMemory, loadedImage.data.get(), static_cast<std::size_t>(loadedImage.size));
   device.unmapMemory(stagingBufferMemory);
   mappedMemory = nullptr;

   copyBufferToImage(stagingBuffer);

   device.destroyBuffer(stagingBuffer);
   stagingBuffer = nullptr;
   device.freeMemory(stagingBufferMemory);
   stagingBufferMemory = nullptr;
}

void Texture::generateMipmaps(vk::ImageLayout finalLayout, const TextureMemoryBarrierFlags& dstMemoryBarrierFlags)
{
   vk::FormatProperties formatProperties = context.getPhysicalDevice().getFormatProperties(imageProperties.format);
   if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
   {
      throw std::runtime_error("Image format " + to_string(imageProperties.format) + " does not support linear blitting");
   }

   vk::CommandBuffer commandBuffer = Command::beginSingle(context);

   vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
      .setAspectMask(textureProperties.aspects)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

   vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
      .setImage(image)
      .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
      .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
      .setSubresourceRange(subresourceRange);

   uint32_t mipWidth = imageProperties.width;
   uint32_t mipHeight = imageProperties.height;
   uint32_t mipDepth = imageProperties.depth;

   for (uint32_t i = 1; i < mipLevels; ++i)
   {
      barrier.subresourceRange.setBaseMipLevel(i - 1);
      barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
      barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
      barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
      barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr, nullptr, { barrier });

      uint32_t nextMipWidth = std::max(mipWidth / 2, 1u);
      uint32_t nextMipHeight = std::max(mipHeight / 2, 1u);
      uint32_t nextMipDepth = std::max(mipDepth / 2, 1u);

      std::array<vk::Offset3D, 2> srcOffsets =
      {
         vk::Offset3D(0, 0, 0),
         vk::Offset3D(mipWidth, mipHeight, mipDepth)
      };

      std::array<vk::Offset3D, 2> dstOffsets =
      {
         vk::Offset3D(0, 0, 0),
         vk::Offset3D(nextMipWidth, nextMipHeight, nextMipDepth)
      };

      vk::ImageSubresourceLayers srcSubresource = vk::ImageSubresourceLayers()
         .setAspectMask(textureProperties.aspects)
         .setMipLevel(i - 1)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::ImageSubresourceLayers dstSubresource = vk::ImageSubresourceLayers()
         .setAspectMask(textureProperties.aspects)
         .setMipLevel(i)
         .setBaseArrayLayer(0)
         .setLayerCount(1);

      vk::ImageBlit blit = vk::ImageBlit()
         .setSrcOffsets(srcOffsets)
         .setSrcSubresource(srcSubresource)
         .setDstOffsets(dstOffsets)
         .setDstSubresource(dstSubresource);

      commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

      if (finalLayout != vk::ImageLayout::eUndefined)
      {
         barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
         barrier.setNewLayout(finalLayout);
         barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
         barrier.setDstAccessMask(dstMemoryBarrierFlags.accessMask);
      }

      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, dstMemoryBarrierFlags.stageMask, vk::DependencyFlags(), nullptr, nullptr, { barrier });

      mipWidth = nextMipWidth;
      mipHeight = nextMipHeight;
      mipDepth = nextMipDepth;
   }

   // Transition the last mip (not handled by the loop)
   if (finalLayout != vk::ImageLayout::eUndefined)
   {
      barrier.subresourceRange.setBaseMipLevel(mipLevels - 1);
      barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
      barrier.setNewLayout(finalLayout);
      barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
      barrier.setDstAccessMask(dstMemoryBarrierFlags.accessMask);
   }

   commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, dstMemoryBarrierFlags.stageMask, vk::DependencyFlags(), nullptr, nullptr, { barrier });

   Command::endSingle(context, commandBuffer);

   layout = finalLayout;
}
