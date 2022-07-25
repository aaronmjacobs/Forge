#include "Graphics/TextureInfo.h"

#include "Graphics/Texture.h"

AttachmentFormats::AttachmentFormats(const Texture* depthStencilAttachment, std::span<const Texture> colorAttachments)
{
   if (depthStencilAttachment)
   {
      depthStencilFormat = depthStencilAttachment->getImageProperties().format;
   }

   colorFormats.reserve(colorAttachments.size());
   for (const Texture& colorAttachment : colorAttachments)
   {
      colorFormats.push_back(colorAttachment.getImageProperties().format);
   }

   if (depthStencilAttachment)
   {
      sampleCount = depthStencilAttachment->getTextureProperties().sampleCount;
   }
   else
   {
      for (const Texture& colorAttachment : colorAttachments)
      {
         sampleCount = colorAttachment.getTextureProperties().sampleCount;
         break;
      }
   }

#if FORGE_DEBUG
   if (depthStencilAttachment)
   {
      ASSERT(sampleCount == depthStencilAttachment->getTextureProperties().sampleCount, "Not all attachments have the same sample count");
   }

   for (const Texture& colorAttachment : colorAttachments)
   {
      ASSERT(sampleCount == colorAttachment.getTextureProperties().sampleCount, "Not all attachments have the same sample count");
   }
#endif // FORGE_DEBUG
}

AttachmentFormats::AttachmentFormats(const Texture* depthStencilAttachment, const Texture* colorAttachment)
   : AttachmentFormats(depthStencilAttachment, colorAttachment ? std::span<const Texture>(colorAttachment, 1) : std::span<const Texture>{})
{
}

namespace FormatHelpers
{
   bool isDepthStencil(vk::Format format)
   {
      switch (format)
      {
      case vk::Format::eD16Unorm:
      case vk::Format::eX8D24UnormPack32:
      case vk::Format::eD32Sfloat:
      case vk::Format::eS8Uint:
      case vk::Format::eD16UnormS8Uint:
      case vk::Format::eD24UnormS8Uint:
      case vk::Format::eD32SfloatS8Uint:
         return true;
      default:
         return false;
      }
   }

   bool hasAlpha(vk::Format format)
   {
      switch (format)
      {
      case vk::Format::eR4G4B4A4UnormPack16:
      case vk::Format::eB4G4R4A4UnormPack16:
      case vk::Format::eR5G5B5A1UnormPack16:
      case vk::Format::eB5G5R5A1UnormPack16:
      case vk::Format::eA1R5G5B5UnormPack16:
      case vk::Format::eR8G8B8A8Unorm:
      case vk::Format::eR8G8B8A8Snorm:
      case vk::Format::eR8G8B8A8Uscaled:
      case vk::Format::eR8G8B8A8Sscaled:
      case vk::Format::eR8G8B8A8Uint:
      case vk::Format::eR8G8B8A8Sint:
      case vk::Format::eR8G8B8A8Srgb:
      case vk::Format::eB8G8R8A8Unorm:
      case vk::Format::eB8G8R8A8Snorm:
      case vk::Format::eB8G8R8A8Uscaled:
      case vk::Format::eB8G8R8A8Sscaled:
      case vk::Format::eB8G8R8A8Uint:
      case vk::Format::eB8G8R8A8Sint:
      case vk::Format::eB8G8R8A8Srgb:
      case vk::Format::eA8B8G8R8UnormPack32:
      case vk::Format::eA8B8G8R8SnormPack32:
      case vk::Format::eA8B8G8R8UscaledPack32:
      case vk::Format::eA8B8G8R8SscaledPack32:
      case vk::Format::eA8B8G8R8UintPack32:
      case vk::Format::eA8B8G8R8SintPack32:
      case vk::Format::eA8B8G8R8SrgbPack32:
      case vk::Format::eA2R10G10B10UnormPack32:
      case vk::Format::eA2R10G10B10SnormPack32:
      case vk::Format::eA2R10G10B10UscaledPack32:
      case vk::Format::eA2R10G10B10SscaledPack32:
      case vk::Format::eA2R10G10B10UintPack32:
      case vk::Format::eA2R10G10B10SintPack32:
      case vk::Format::eA2B10G10R10UnormPack32:
      case vk::Format::eA2B10G10R10SnormPack32:
      case vk::Format::eA2B10G10R10UscaledPack32:
      case vk::Format::eA2B10G10R10SscaledPack32:
      case vk::Format::eA2B10G10R10UintPack32:
      case vk::Format::eA2B10G10R10SintPack32:
      case vk::Format::eR16G16B16A16Unorm:
      case vk::Format::eR16G16B16A16Snorm:
      case vk::Format::eR16G16B16A16Uscaled:
      case vk::Format::eR16G16B16A16Sscaled:
      case vk::Format::eR16G16B16A16Uint:
      case vk::Format::eR16G16B16A16Sint:
      case vk::Format::eR16G16B16A16Sfloat:
      case vk::Format::eR32G32B32A32Uint:
      case vk::Format::eR32G32B32A32Sint:
      case vk::Format::eR32G32B32A32Sfloat:
      case vk::Format::eR64G64B64A64Uint:
      case vk::Format::eR64G64B64A64Sint:
      case vk::Format::eR64G64B64A64Sfloat:
      case vk::Format::eBc1RgbaUnormBlock:
      case vk::Format::eBc1RgbaSrgbBlock:
      case vk::Format::eBc2UnormBlock:
      case vk::Format::eBc2SrgbBlock:
      case vk::Format::eBc3UnormBlock:
      case vk::Format::eBc3SrgbBlock:
      case vk::Format::eBc7UnormBlock:
      case vk::Format::eBc7SrgbBlock:
      case vk::Format::eEtc2R8G8B8A1UnormBlock:
      case vk::Format::eEtc2R8G8B8A1SrgbBlock:
      case vk::Format::eEtc2R8G8B8A8UnormBlock:
      case vk::Format::eEtc2R8G8B8A8SrgbBlock:
      case vk::Format::eAstc4x4UnormBlock:
      case vk::Format::eAstc4x4SrgbBlock:
      case vk::Format::eAstc5x4UnormBlock:
      case vk::Format::eAstc5x4SrgbBlock:
      case vk::Format::eAstc5x5UnormBlock:
      case vk::Format::eAstc5x5SrgbBlock:
      case vk::Format::eAstc6x5UnormBlock:
      case vk::Format::eAstc6x5SrgbBlock:
      case vk::Format::eAstc6x6UnormBlock:
      case vk::Format::eAstc6x6SrgbBlock:
      case vk::Format::eAstc8x5UnormBlock:
      case vk::Format::eAstc8x5SrgbBlock:
      case vk::Format::eAstc8x6UnormBlock:
      case vk::Format::eAstc8x6SrgbBlock:
      case vk::Format::eAstc8x8UnormBlock:
      case vk::Format::eAstc8x8SrgbBlock:
      case vk::Format::eAstc10x5UnormBlock:
      case vk::Format::eAstc10x5SrgbBlock:
      case vk::Format::eAstc10x6UnormBlock:
      case vk::Format::eAstc10x6SrgbBlock:
      case vk::Format::eAstc10x8UnormBlock:
      case vk::Format::eAstc10x8SrgbBlock:
      case vk::Format::eAstc10x10UnormBlock:
      case vk::Format::eAstc10x10SrgbBlock:
      case vk::Format::eAstc12x10UnormBlock:
      case vk::Format::eAstc12x10SrgbBlock:
      case vk::Format::eAstc12x12UnormBlock:
      case vk::Format::eAstc12x12SrgbBlock:
      case vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16:
      case vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16:
      case vk::Format::eAstc4x4SfloatBlockEXT:
      case vk::Format::eAstc5x4SfloatBlockEXT:
      case vk::Format::eAstc5x5SfloatBlockEXT:
      case vk::Format::eAstc6x5SfloatBlockEXT:
      case vk::Format::eAstc6x6SfloatBlockEXT:
      case vk::Format::eAstc8x5SfloatBlockEXT:
      case vk::Format::eAstc8x6SfloatBlockEXT:
      case vk::Format::eAstc8x8SfloatBlockEXT:
      case vk::Format::eAstc10x5SfloatBlockEXT:
      case vk::Format::eAstc10x6SfloatBlockEXT:
      case vk::Format::eAstc10x8SfloatBlockEXT:
      case vk::Format::eAstc10x10SfloatBlockEXT:
      case vk::Format::eAstc12x10SfloatBlockEXT:
      case vk::Format::eAstc12x12SfloatBlockEXT:
      case vk::Format::eA4R4G4B4UnormPack16EXT:
      case vk::Format::eA4B4G4R4UnormPack16EXT:
         return true;
      default:
         return false;
      }
   }

   uint32_t bitsPerPixel(vk::Format format)
   {
      switch (format)
      {
      case vk::Format::eBc1RgbUnormBlock:
      case vk::Format::eBc1RgbSrgbBlock:
      case vk::Format::eBc1RgbaUnormBlock:
      case vk::Format::eBc1RgbaSrgbBlock:
      case vk::Format::eBc4UnormBlock:
      case vk::Format::eBc4SnormBlock:
         return 4;
      case vk::Format::eR4G4UnormPack8:
      case vk::Format::eR8Unorm:
      case vk::Format::eR8Snorm:
      case vk::Format::eR8Uscaled:
      case vk::Format::eR8Sscaled:
      case vk::Format::eR8Uint:
      case vk::Format::eR8Sint:
      case vk::Format::eR8Srgb:
      case vk::Format::eS8Uint:
      case vk::Format::eBc2UnormBlock:
      case vk::Format::eBc2SrgbBlock:
      case vk::Format::eBc3UnormBlock:
      case vk::Format::eBc3SrgbBlock:
      case vk::Format::eBc5UnormBlock:
      case vk::Format::eBc5SnormBlock:
      case vk::Format::eBc6HUfloatBlock:
      case vk::Format::eBc6HSfloatBlock:
      case vk::Format::eBc7UnormBlock:
      case vk::Format::eBc7SrgbBlock:
         return 8;
      case vk::Format::eR4G4B4A4UnormPack16:
      case vk::Format::eB4G4R4A4UnormPack16:
      case vk::Format::eR5G6B5UnormPack16:
      case vk::Format::eB5G6R5UnormPack16:
      case vk::Format::eR5G5B5A1UnormPack16:
      case vk::Format::eB5G5R5A1UnormPack16:
      case vk::Format::eA1R5G5B5UnormPack16:
      case vk::Format::eR8G8Unorm:
      case vk::Format::eR8G8Snorm:
      case vk::Format::eR8G8Uscaled:
      case vk::Format::eR8G8Sscaled:
      case vk::Format::eR8G8Uint:
      case vk::Format::eR8G8Sint:
      case vk::Format::eR8G8Srgb:
      case vk::Format::eR16Unorm:
      case vk::Format::eR16Snorm:
      case vk::Format::eR16Uscaled:
      case vk::Format::eR16Sscaled:
      case vk::Format::eR16Uint:
      case vk::Format::eR16Sint:
      case vk::Format::eR16Sfloat:
      case vk::Format::eD16Unorm:
      case vk::Format::eR10X6UnormPack16:
      case vk::Format::eR10X6G10X6Unorm2Pack16:
      case vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16:
      case vk::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16:
      case vk::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16:
      case vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16:
      case vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16:
      case vk::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16:
      case vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16:
      case vk::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16:
      case vk::Format::eR12X4UnormPack16:
      case vk::Format::eR12X4G12X4Unorm2Pack16:
      case vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16:
      case vk::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16:
      case vk::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16:
      case vk::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16:
      case vk::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16:
      case vk::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16:
      case vk::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16:
      case vk::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16:
      case vk::Format::eA4R4G4B4UnormPack16EXT:
      case vk::Format::eA4B4G4R4UnormPack16EXT:
         return 16;
      case vk::Format::eR8G8B8Unorm:
      case vk::Format::eR8G8B8Snorm:
      case vk::Format::eR8G8B8Uscaled:
      case vk::Format::eR8G8B8Sscaled:
      case vk::Format::eR8G8B8Uint:
      case vk::Format::eR8G8B8Sint:
      case vk::Format::eR8G8B8Srgb:
      case vk::Format::eB8G8R8Unorm:
      case vk::Format::eB8G8R8Snorm:
      case vk::Format::eB8G8R8Uscaled:
      case vk::Format::eB8G8R8Sscaled:
      case vk::Format::eB8G8R8Uint:
      case vk::Format::eB8G8R8Sint:
      case vk::Format::eB8G8R8Srgb:
      case vk::Format::eD16UnormS8Uint:
         return 24;
      case vk::Format::eR8G8B8A8Unorm:
      case vk::Format::eR8G8B8A8Snorm:
      case vk::Format::eR8G8B8A8Uscaled:
      case vk::Format::eR8G8B8A8Sscaled:
      case vk::Format::eR8G8B8A8Uint:
      case vk::Format::eR8G8B8A8Sint:
      case vk::Format::eR8G8B8A8Srgb:
      case vk::Format::eB8G8R8A8Unorm:
      case vk::Format::eB8G8R8A8Snorm:
      case vk::Format::eB8G8R8A8Uscaled:
      case vk::Format::eB8G8R8A8Sscaled:
      case vk::Format::eB8G8R8A8Uint:
      case vk::Format::eB8G8R8A8Sint:
      case vk::Format::eB8G8R8A8Srgb:
      case vk::Format::eA8B8G8R8UnormPack32:
      case vk::Format::eA8B8G8R8SnormPack32:
      case vk::Format::eA8B8G8R8UscaledPack32:
      case vk::Format::eA8B8G8R8SscaledPack32:
      case vk::Format::eA8B8G8R8UintPack32:
      case vk::Format::eA8B8G8R8SintPack32:
      case vk::Format::eA8B8G8R8SrgbPack32:
      case vk::Format::eA2R10G10B10UnormPack32:
      case vk::Format::eA2R10G10B10SnormPack32:
      case vk::Format::eA2R10G10B10UscaledPack32:
      case vk::Format::eA2R10G10B10SscaledPack32:
      case vk::Format::eA2R10G10B10UintPack32:
      case vk::Format::eA2R10G10B10SintPack32:
      case vk::Format::eA2B10G10R10UnormPack32:
      case vk::Format::eA2B10G10R10SnormPack32:
      case vk::Format::eA2B10G10R10UscaledPack32:
      case vk::Format::eA2B10G10R10SscaledPack32:
      case vk::Format::eA2B10G10R10UintPack32:
      case vk::Format::eA2B10G10R10SintPack32:
      case vk::Format::eR16G16Unorm:
      case vk::Format::eR16G16Snorm:
      case vk::Format::eR16G16Uscaled:
      case vk::Format::eR16G16Sscaled:
      case vk::Format::eR16G16Uint:
      case vk::Format::eR16G16Sint:
      case vk::Format::eR16G16Sfloat:
      case vk::Format::eR32Uint:
      case vk::Format::eR32Sint:
      case vk::Format::eR32Sfloat:
      case vk::Format::eB10G11R11UfloatPack32:
      case vk::Format::eE5B9G9R9UfloatPack32:
      case vk::Format::eX8D24UnormPack32:
      case vk::Format::eD32Sfloat:
      case vk::Format::eD24UnormS8Uint:
      case vk::Format::eG8B8G8R8422Unorm:
      case vk::Format::eB8G8R8G8422Unorm:
         return 32;
      case vk::Format::eD32SfloatS8Uint:
         return 40;
      case vk::Format::eR16G16B16Unorm:
      case vk::Format::eR16G16B16Snorm:
      case vk::Format::eR16G16B16Uscaled:
      case vk::Format::eR16G16B16Sscaled:
      case vk::Format::eR16G16B16Uint:
      case vk::Format::eR16G16B16Sint:
      case vk::Format::eR16G16B16Sfloat:
         return 48;
      case vk::Format::eR16G16B16A16Unorm:
      case vk::Format::eR16G16B16A16Snorm:
      case vk::Format::eR16G16B16A16Uscaled:
      case vk::Format::eR16G16B16A16Sscaled:
      case vk::Format::eR16G16B16A16Uint:
      case vk::Format::eR16G16B16A16Sint:
      case vk::Format::eR16G16B16A16Sfloat:
      case vk::Format::eR32G32Uint:
      case vk::Format::eR32G32Sint:
      case vk::Format::eR32G32Sfloat:
      case vk::Format::eR64Uint:
      case vk::Format::eR64Sint:
      case vk::Format::eR64Sfloat:
      case vk::Format::eG16B16G16R16422Unorm:
      case vk::Format::eB16G16R16G16422Unorm:
         return 64;
      case vk::Format::eR32G32B32Uint:
      case vk::Format::eR32G32B32Sint:
      case vk::Format::eR32G32B32Sfloat:
         return 96;
      case vk::Format::eR32G32B32A32Uint:
      case vk::Format::eR32G32B32A32Sint:
      case vk::Format::eR32G32B32A32Sfloat:
      case vk::Format::eR64G64Uint:
      case vk::Format::eR64G64Sint:
      case vk::Format::eR64G64Sfloat:
         return 128;
      case vk::Format::eR64G64B64Uint:
      case vk::Format::eR64G64B64Sint:
      case vk::Format::eR64G64B64Sfloat:
         return 192;
      case vk::Format::eR64G64B64A64Uint:
      case vk::Format::eR64G64B64A64Sint:
      case vk::Format::eR64G64B64A64Sfloat:
         return 256;
      default:
         return 0;
      }
   }

   uint32_t bytesPerBlock(vk::Format format)
   {
      switch (format)
      {
      case vk::Format::eBc1RgbUnormBlock:
      case vk::Format::eBc1RgbSrgbBlock:
      case vk::Format::eBc1RgbaUnormBlock:
      case vk::Format::eBc1RgbaSrgbBlock:
      case vk::Format::eBc4UnormBlock:
      case vk::Format::eBc4SnormBlock:
         return 8;
      case vk::Format::eBc2UnormBlock:
      case vk::Format::eBc2SrgbBlock:
      case vk::Format::eBc3UnormBlock:
      case vk::Format::eBc3SrgbBlock:
      case vk::Format::eBc5UnormBlock:
      case vk::Format::eBc5SnormBlock:
      case vk::Format::eBc6HUfloatBlock:
      case vk::Format::eBc6HSfloatBlock:
      case vk::Format::eBc7UnormBlock:
      case vk::Format::eBc7SrgbBlock:
         return 16;
      default:
         return 0;
      }
   }
}
