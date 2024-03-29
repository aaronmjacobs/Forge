#include "Graphics/Texture.h"

#include "Core/Assert.h"

#include "Graphics/Buffer.h"
#include "Graphics/Command.h"
#include "Graphics/DebugUtils.h"
#include "Graphics/Memory.h"

#include <cmath>
#include <utility>

namespace
{
   vk::ImageViewType getDefaultViewType(const ImageProperties& imageProperties)
   {
      if (imageProperties.cubeCompatible)
      {
         return imageProperties.layers <= 6 ? vk::ImageViewType::eCube : vk::ImageViewType::eCubeArray;
      }
      else
      {
         switch (imageProperties.type)
         {
         case vk::ImageType::e1D:
            return imageProperties.layers == 1 ? vk::ImageViewType::e1D : vk::ImageViewType::e1DArray;
         case vk::ImageType::e2D:
            return imageProperties.layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
         case vk::ImageType::e3D:
            return vk::ImageViewType::e3D;
         default:
            ASSERT(false);
            return vk::ImageViewType::e2D;
         }
      }
   }

   uint32_t calcMipLevels(const ImageProperties& imageProperties)
   {
      uint32_t maxDimension = std::max(imageProperties.width, std::max(imageProperties.height, imageProperties.depth));
      return static_cast<uint32_t>(std::log2(maxDimension)) + 1;
   }

   vk::ImageLayout getImageLayout(TextureLayoutType layoutType, bool isDepthStencil)
   {
      switch (layoutType)
      {
      case TextureLayoutType::AttachmentWrite:
         return isDepthStencil ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eColorAttachmentOptimal;
      case TextureLayoutType::ShaderRead:
         return vk::ImageLayout::eShaderReadOnlyOptimal;
      case TextureLayoutType::Present:
         return vk::ImageLayout::ePresentSrcKHR;
      default:
         ASSERT(false);
         return vk::ImageLayout::eUndefined;
      }
   }

   TextureMemoryBarrierFlags getSrcMemoryBarrierFlags(vk::ImageLayout layout)
   {
      switch (layout)
      {
      case vk::ImageLayout::eUndefined:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eNone, vk::PipelineStageFlagBits::eTopOfPipe);
      case vk::ImageLayout::eColorAttachmentOptimal:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);
      case vk::ImageLayout::eDepthStencilAttachmentOptimal:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eLateFragmentTests);
      case vk::ImageLayout::eShaderReadOnlyOptimal:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      case vk::ImageLayout::ePresentSrcKHR:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eNone, vk::PipelineStageFlagBits::eTopOfPipe);
      default:
         ASSERT(false);
         return TextureMemoryBarrierFlags{};
      }
   }

   TextureMemoryBarrierFlags getDstMemoryBarrierFlags(TextureLayoutType layoutType, bool isDepthStencil)
   {
      switch (layoutType)
      {
      case TextureLayoutType::AttachmentWrite:
         return isDepthStencil ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eLateFragmentTests) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput) ;
      case TextureLayoutType::ShaderRead:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
      case TextureLayoutType::Present:
         return TextureMemoryBarrierFlags(vk::AccessFlagBits::eNone, vk::PipelineStageFlagBits::eBottomOfPipe);
      default:
         ASSERT(false);
         return TextureMemoryBarrierFlags{};
      }
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

Texture::Texture(const GraphicsContext& graphicsContext, const ImageProperties& imageProps, const TextureProperties& textureProps, const TextureInitialLayout& initialLayout, const TextureData& textureData)
   : GraphicsResource(graphicsContext)
   , imageProperties(imageProps)
   , textureProperties(textureProps)
{
   bool hasTextureData = !textureData.bytes.empty() && !textureData.mips.empty() && textureData.mipsPerLayer > 0;
   if (hasTextureData)
   {
      if (textureData.mipsPerLayer > 1)
      {
         mipLevels = textureData.mipsPerLayer;
         textureProperties.generateMipMaps = false;
      }
      else if (textureProperties.generateMipMaps)
      {
         mipLevels = calcMipLevels(imageProperties);
         if (mipLevels < 2)
         {
            textureProperties.generateMipMaps = false;
         }
      }

      textureProperties.usage |= vk::ImageUsageFlagBits::eTransferDst;
      if (textureProperties.generateMipMaps)
      {
         textureProperties.usage |= vk::ImageUsageFlagBits::eTransferSrc;
      }
   }

   createImage();
   createDefaultView();

   if (hasTextureData)
   {
      stageAndCopyImage(textureData);

      if (textureProperties.generateMipMaps)
      {
         generateMipmaps(initialLayout.layout, initialLayout.memoryBarrierFlags);
      }
      else
      {
         transitionLayout(nullptr, initialLayout.layout, TextureMemoryBarrierFlags(vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer), initialLayout.memoryBarrierFlags);
      }
   }
   else
   {
      transitionLayout(nullptr, initialLayout.layout, TextureMemoryBarrierFlags(vk::AccessFlags(), vk::PipelineStageFlagBits::eTopOfPipe), initialLayout.memoryBarrierFlags);
   }
}

Texture::Texture(const GraphicsContext& graphicsContext, const ImageProperties& imageProps, vk::Image swapchainImage)
   : GraphicsResource(graphicsContext)
   , image(swapchainImage)
   , imageProperties(imageProps)
{
   textureProperties.sampleCount = vk::SampleCountFlagBits::e1;
   textureProperties.tiling = vk::ImageTiling::eOptimal;
   textureProperties.usage = vk::ImageUsageFlagBits::eColorAttachment;
   textureProperties.aspects = vk::ImageAspectFlagBits::eColor;

   createDefaultView();
}

Texture::~Texture()
{
   for (auto& [desc, view] : viewMap)
   {
      ASSERT(view);
      context.delayedDestroy(std::move(view));
   }

   if (imageAllocation)
   {
      ASSERT(image);
      context.delayedDestroy(std::move(image), std::move(imageAllocation));
   }
}

vk::ImageView Texture::getOrCreateView(vk::ImageViewType viewType, uint32_t baseLayer, uint32_t layerCount, std::optional<vk::ImageAspectFlags> aspectFlags, bool* created)
{
   ImageViewDesc desc;
   desc.viewType = viewType;
   desc.baseLayer = baseLayer;
   desc.layerCount = layerCount;
   desc.aspectFlags = aspectFlags.value_or(textureProperties.aspects);

   auto location = viewMap.find(desc);
   if (location != viewMap.end())
   {
      if (created)
      {
         *created = false;
      }
      return location->second;
   }

   vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
      .setAspectMask(aspectFlags.value_or(textureProperties.aspects))
      .setBaseMipLevel(0)
      .setLevelCount(mipLevels)
      .setBaseArrayLayer(baseLayer)
      .setLayerCount(layerCount);

   vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
      .setImage(image)
      .setViewType(viewType)
      .setFormat(imageProperties.format)
      .setSubresourceRange(subresourceRange);

   vk::ImageView view = device.createImageView(createInfo);
   NAME_CHILD(view, "View " + DebugUtils::toString(viewMap.size()));
   viewMap[desc] = view;

   if (created)
   {
      *created = true;
   }
   return view;
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
         .setLayerCount(imageProperties.layers);

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

void Texture::transitionLayout(vk::CommandBuffer commandBuffer, TextureLayoutType layoutType)
{
   bool isDepthStencil = FormatHelpers::isDepthStencil(imageProperties.format);

   vk::ImageLayout newLayout = getImageLayout(layoutType, isDepthStencil);
   if (layout != newLayout)
   {
      TextureMemoryBarrierFlags srcMemoryBarrierFlags = getSrcMemoryBarrierFlags(layout);
      TextureMemoryBarrierFlags dstMemoryBarrierFlags = getDstMemoryBarrierFlags(layoutType, isDepthStencil);

      transitionLayout(commandBuffer, newLayout, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   }
}

void Texture::createImage()
{
   ASSERT(!image && !imageAllocation);

   vk::ImageCreateInfo imageCreateinfo = vk::ImageCreateInfo()
      .setImageType(imageProperties.type)
      .setExtent(vk::Extent3D(imageProperties.width, imageProperties.height, imageProperties.depth))
      .setMipLevels(mipLevels)
      .setArrayLayers(imageProperties.layers)
      .setFormat(imageProperties.format)
      .setTiling(textureProperties.tiling)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setUsage(textureProperties.usage)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setSamples(textureProperties.sampleCount);
   if (imageProperties.cubeCompatible)
   {
      imageCreateinfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
   }

   VmaAllocationCreateInfo imageAllocationCreateInfo{};
   imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

   VkImage vkImage = nullptr;
   if (vmaCreateImage(context.getVmaAllocator(), &static_cast<VkImageCreateInfo&>(imageCreateinfo), &imageAllocationCreateInfo, &vkImage, &imageAllocation, nullptr) != VK_SUCCESS)
   {
      throw new std::runtime_error("Failed to create image");
   }
   image = vkImage;
   NAME_CHILD(image, "Image");
}

void Texture::createDefaultView()
{
   ASSERT(!defaultView);

   defaultView = getOrCreateView(getDefaultViewType(imageProperties), 0, imageProperties.cubeCompatible ? 6 : 1);
   NAME_CHILD(defaultView, "Default View");
}

void Texture::copyBufferToImage(vk::Buffer buffer, const TextureData& textureData)
{
   vk::CommandBuffer commandBuffer = Command::beginSingle(context);

   std::vector<vk::BufferImageCopy> regions;
   regions.reserve(textureData.mips.size());

   ASSERT(textureData.mips.size() % textureData.mipsPerLayer == 0);
   uint32_t numLayers = static_cast<uint32_t>(textureData.mips.size() / textureData.mipsPerLayer);

   for (uint32_t layer = 0; layer < numLayers; ++layer)
   {
      for (uint32_t mipLevel = 0; mipLevel < textureData.mipsPerLayer; ++mipLevel)
      {
         const MipInfo& mipInfo = textureData.mips[layer * textureData.mipsPerLayer + mipLevel];

         vk::ImageSubresourceLayers imageSubresource = vk::ImageSubresourceLayers()
            .setAspectMask(textureProperties.aspects)
            .setMipLevel(mipLevel)
            .setBaseArrayLayer(layer)
            .setLayerCount(1);

         vk::BufferImageCopy region = vk::BufferImageCopy()
            .setBufferOffset(mipInfo.bufferOffset)
            .setBufferRowLength(0)
            .setBufferImageHeight(0)
            .setImageSubresource(imageSubresource)
            .setImageOffset(vk::Offset3D(0, 0, 0))
            .setImageExtent(mipInfo.extent);

         regions.push_back(region);
      }
   }

   layout = vk::ImageLayout::eTransferDstOptimal;
   commandBuffer.copyBufferToImage(buffer, image, layout, regions);

   Command::endSingle(context, commandBuffer);
}

void Texture::stageAndCopyImage(const TextureData& textureData)
{
   transitionLayout(nullptr, vk::ImageLayout::eTransferDstOptimal, TextureMemoryBarrierFlags(vk::AccessFlags(), vk::PipelineStageFlagBits::eTopOfPipe), TextureMemoryBarrierFlags(vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer));

   vk::Buffer stagingBuffer;
   VmaAllocation stagingBufferAllocation = nullptr;
   void* mappedMemory = nullptr;
   Buffer::create(context, textureData.bytes.size_bytes(), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation, &mappedMemory);

   std::memcpy(mappedMemory, textureData.bytes.data(), textureData.bytes.size_bytes());
   mappedMemory = nullptr;

   copyBufferToImage(stagingBuffer, textureData);

   vmaDestroyBuffer(context.getVmaAllocator(), stagingBuffer, stagingBufferAllocation);
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
      .setLayerCount(imageProperties.layers);

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
         .setLayerCount(imageProperties.layers);

      vk::ImageSubresourceLayers dstSubresource = vk::ImageSubresourceLayers()
         .setAspectMask(textureProperties.aspects)
         .setMipLevel(i)
         .setBaseArrayLayer(0)
         .setLayerCount(imageProperties.layers);

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
