#pragma once

#include "Graphics/Vulkan.h"

#include <optional>
#include <span>
#include <vector>

class Texture;

struct AttachmentFormats
{
   vk::Format depthStencilFormat = vk::Format::eUndefined;
   std::vector<vk::Format> colorFormats;

   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

   AttachmentFormats() = default;
   AttachmentFormats(const Texture* depthStencilAttachment, std::span<const Texture> colorAttachments);
   AttachmentFormats(const Texture* depthStencilAttachment, const Texture* colorAttachment);

   bool operator==(const AttachmentFormats& other) const = default;
};

struct ImageProperties
{
   vk::Format format = vk::Format::eR8G8B8A8Srgb;
   vk::ImageType type = vk::ImageType::e2D;
   uint32_t width = 1;
   uint32_t height = 1;
   uint32_t depth = 1;
   uint32_t layers = 1;
   bool hasAlpha = false;
   bool cubeCompatible = false;
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

struct MipInfo
{
   vk::Extent3D extent;
   uint32_t bufferOffset = 0;
};

struct TextureData
{
   std::span<const uint8_t> bytes;
   std::span<const MipInfo> mips;
   uint32_t mipsPerLayer = 0;
};

namespace FormatHelpers
{
   bool isDepthStencil(vk::Format format);
   bool hasAlpha(vk::Format format);
   uint32_t bitsPerPixel(vk::Format format);
   uint32_t bytesPerBlock(vk::Format format);
}
