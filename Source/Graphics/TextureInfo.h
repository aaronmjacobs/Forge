#pragma once

#include "Graphics/Vulkan.h"

#include <optional>
#include <vector>

struct BasicTextureInfo
{
   vk::Format format = vk::Format::eUndefined;
   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

   bool isSwapchainTexture = false;

   bool operator==(const BasicTextureInfo& other) const = default;
};

struct TextureInfo
{
   vk::Format format = vk::Format::eUndefined;
   vk::Extent2D extent;
   vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;

   vk::ImageView view;
   bool isSwapchainTexture = false;

   BasicTextureInfo asBasic() const;
   bool operator==(const TextureInfo& other) const = default;
};

struct BasicAttachmentInfo
{
   std::optional<BasicTextureInfo> depthInfo;
   std::vector<BasicTextureInfo> colorInfo;
   std::vector<BasicTextureInfo> resolveInfo;

   bool operator==(const BasicAttachmentInfo& other) const = default;
};

struct AttachmentInfo
{
   std::optional<TextureInfo> depthInfo;
   std::vector<TextureInfo> colorInfo;
   std::vector<TextureInfo> resolveInfo;

   BasicAttachmentInfo asBasic() const;
   bool operator==(const AttachmentInfo& other) const = default;
};
