#include "Resources/DDSImageLoader.h"

#include "Resources/Image.h"

#include <utility>

namespace
{
   enum DXGIFormat
   {
      DXGI_FORMAT_UNKNOWN = 0,
      DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
      DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
      DXGI_FORMAT_R32G32B32A32_UINT = 3,
      DXGI_FORMAT_R32G32B32A32_SINT = 4,
      DXGI_FORMAT_R32G32B32_TYPELESS = 5,
      DXGI_FORMAT_R32G32B32_FLOAT = 6,
      DXGI_FORMAT_R32G32B32_UINT = 7,
      DXGI_FORMAT_R32G32B32_SINT = 8,
      DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
      DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
      DXGI_FORMAT_R16G16B16A16_UNORM = 11,
      DXGI_FORMAT_R16G16B16A16_UINT = 12,
      DXGI_FORMAT_R16G16B16A16_SNORM = 13,
      DXGI_FORMAT_R16G16B16A16_SINT = 14,
      DXGI_FORMAT_R32G32_TYPELESS = 15,
      DXGI_FORMAT_R32G32_FLOAT = 16,
      DXGI_FORMAT_R32G32_UINT = 17,
      DXGI_FORMAT_R32G32_SINT = 18,
      DXGI_FORMAT_R32G8X24_TYPELESS = 19,
      DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
      DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
      DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
      DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
      DXGI_FORMAT_R10G10B10A2_UNORM = 24,
      DXGI_FORMAT_R10G10B10A2_UINT = 25,
      DXGI_FORMAT_R11G11B10_FLOAT = 26,
      DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
      DXGI_FORMAT_R8G8B8A8_UNORM = 28,
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
      DXGI_FORMAT_R8G8B8A8_UINT = 30,
      DXGI_FORMAT_R8G8B8A8_SNORM = 31,
      DXGI_FORMAT_R8G8B8A8_SINT = 32,
      DXGI_FORMAT_R16G16_TYPELESS = 33,
      DXGI_FORMAT_R16G16_FLOAT = 34,
      DXGI_FORMAT_R16G16_UNORM = 35,
      DXGI_FORMAT_R16G16_UINT = 36,
      DXGI_FORMAT_R16G16_SNORM = 37,
      DXGI_FORMAT_R16G16_SINT = 38,
      DXGI_FORMAT_R32_TYPELESS = 39,
      DXGI_FORMAT_D32_FLOAT = 40,
      DXGI_FORMAT_R32_FLOAT = 41,
      DXGI_FORMAT_R32_UINT = 42,
      DXGI_FORMAT_R32_SINT = 43,
      DXGI_FORMAT_R24G8_TYPELESS = 44,
      DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
      DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
      DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
      DXGI_FORMAT_R8G8_TYPELESS = 48,
      DXGI_FORMAT_R8G8_UNORM = 49,
      DXGI_FORMAT_R8G8_UINT = 50,
      DXGI_FORMAT_R8G8_SNORM = 51,
      DXGI_FORMAT_R8G8_SINT = 52,
      DXGI_FORMAT_R16_TYPELESS = 53,
      DXGI_FORMAT_R16_FLOAT = 54,
      DXGI_FORMAT_D16_UNORM = 55,
      DXGI_FORMAT_R16_UNORM = 56,
      DXGI_FORMAT_R16_UINT = 57,
      DXGI_FORMAT_R16_SNORM = 58,
      DXGI_FORMAT_R16_SINT = 59,
      DXGI_FORMAT_R8_TYPELESS = 60,
      DXGI_FORMAT_R8_UNORM = 61,
      DXGI_FORMAT_R8_UINT = 62,
      DXGI_FORMAT_R8_SNORM = 63,
      DXGI_FORMAT_R8_SINT = 64,
      DXGI_FORMAT_A8_UNORM = 65,
      DXGI_FORMAT_R1_UNORM = 66,
      DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
      DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
      DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
      DXGI_FORMAT_BC1_TYPELESS = 70,
      DXGI_FORMAT_BC1_UNORM = 71,
      DXGI_FORMAT_BC1_UNORM_SRGB = 72,
      DXGI_FORMAT_BC2_TYPELESS = 73,
      DXGI_FORMAT_BC2_UNORM = 74,
      DXGI_FORMAT_BC2_UNORM_SRGB = 75,
      DXGI_FORMAT_BC3_TYPELESS = 76,
      DXGI_FORMAT_BC3_UNORM = 77,
      DXGI_FORMAT_BC3_UNORM_SRGB = 78,
      DXGI_FORMAT_BC4_TYPELESS = 79,
      DXGI_FORMAT_BC4_UNORM = 80,
      DXGI_FORMAT_BC4_SNORM = 81,
      DXGI_FORMAT_BC5_TYPELESS = 82,
      DXGI_FORMAT_BC5_UNORM = 83,
      DXGI_FORMAT_BC5_SNORM = 84,
      DXGI_FORMAT_B5G6R5_UNORM = 85,
      DXGI_FORMAT_B5G5R5A1_UNORM = 86,
      DXGI_FORMAT_B8G8R8A8_UNORM = 87,
      DXGI_FORMAT_B8G8R8X8_UNORM = 88,
      DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
      DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
      DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
      DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
      DXGI_FORMAT_BC6H_TYPELESS = 94,
      DXGI_FORMAT_BC6H_UF16 = 95,
      DXGI_FORMAT_BC6H_SF16 = 96,
      DXGI_FORMAT_BC7_TYPELESS = 97,
      DXGI_FORMAT_BC7_UNORM = 98,
      DXGI_FORMAT_BC7_UNORM_SRGB = 99,
      DXGI_FORMAT_AYUV = 100,
      DXGI_FORMAT_Y410 = 101,
      DXGI_FORMAT_Y416 = 102,
      DXGI_FORMAT_NV12 = 103,
      DXGI_FORMAT_P010 = 104,
      DXGI_FORMAT_P016 = 105,
      DXGI_FORMAT_420_OPAQUE = 106,
      DXGI_FORMAT_YUY2 = 107,
      DXGI_FORMAT_Y210 = 108,
      DXGI_FORMAT_Y216 = 109,
      DXGI_FORMAT_NV11 = 110,
      DXGI_FORMAT_AI44 = 111,
      DXGI_FORMAT_IA44 = 112,
      DXGI_FORMAT_P8 = 113,
      DXGI_FORMAT_A8P8 = 114,
      DXGI_FORMAT_B4G4R4A4_UNORM = 115,

      DXGI_FORMAT_P208 = 130,
      DXGI_FORMAT_V208 = 131,
      DXGI_FORMAT_V408 = 132,

      DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
      DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,

      DXGI_FORMAT_FORCE_UINT = 0xffffffff
   };

   enum D3D10ResourceDimension
   {
      D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
      D3D10_RESOURCE_DIMENSION_BUFFER = 1,
      D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
      D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
      D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
   };

   namespace DDSFlags
   {
      enum Enum : uint32_t
      {
         None = 0x0,
         Caps = 0x1,
         Height = 0x2,
         Width = 0x4,
         Pitch = 0x8,
         PixelFormat = 0x1000,
         MipMapCount = 0x20000,
         LinearSize = 0x80000,
         Depth = 0x800000
      };
   }

   namespace DDSCaps
   {
      enum Enum : uint32_t
      {
         None = 0x0,
         Complex = 0x8,
         MipMap = 0x400000,
         Texture = 0x1000
      };
   }

   namespace DDSCaps2
   {
      enum Enum : uint32_t
      {
         None = 0x0,
         Cubemap = 0x200,
         CubemapPositiveX = 0x400,
         CubemapNegativeX = 0x800,
         CubemapPositiveY = 0x1000,
         CubemapNegativeY = 0x2000,
         CubemapPositiveZ = 0x4000,
         CubemapNegativeZ = 0x8000,
         Volume = 0x200000
      };
   }

   namespace DDSPixelFormatFlags
   {
      enum Enum : uint32_t
      {
         None = 0x0,
         AlphaPixels = 0x1,
         Alpha = 0x2,
         FourCC = 0x4,
         RGB = 0x40,
         YUV = 0x200,
         Luminance = 0x20000
      };
   }

   constexpr uint32_t fourCC(const char chars[5])
   {
      return static_cast<uint32_t>((chars[0] << 0) | (chars[1] << 8) | (chars[2] << 16) | (chars[3] << 24));
   }

   namespace DDSFourCC
   {
      enum Enum : uint32_t
      {
         Invalid = 0,

         DXT1 = fourCC("DXT1"),
         DXT2 = fourCC("DXT2"),
         DXT3 = fourCC("DXT3"),
         DXT4 = fourCC("DXT4"),
         DXT5 = fourCC("DXT5"),

         ATI1 = fourCC("ATI1"),
         ATI2 = fourCC("ATI2"),

         BC4U = fourCC("BC4U"),
         BC4S = fourCC("BC4S"),
         BC5U = fourCC("BC5U"),
         BC5S = fourCC("BC5S"),

         RGBG = fourCC("RGBG"),
         GRGB = fourCC("GRGB"),

         DX10 = fourCC("DX10")
      };
   }

   namespace DDSDX10MiscFlag
   {
      enum Enum : uint32_t
      {
         None = 0x0,
         TextureCube = 0x4
      };
   }

   namespace DDSDX10AlphaMode
   {
      enum Enum : uint32_t
      {
         Unknown = 0x0,
         Straight = 0x1,
         Premultiplied = 0x2,
         Opaque = 0x3,
         Custom = 0x4
      };
   }

   struct DDSPixelFormat
   {
      uint32_t size = 0;
      DDSPixelFormatFlags::Enum flags = DDSPixelFormatFlags::None;
      DDSFourCC::Enum fourCC = DDSFourCC::Invalid;
      uint32_t rgbBitCount = 0;
      uint32_t rBitMask = 0;
      uint32_t gBitMask = 0;
      uint32_t bBitMask = 0;
      uint32_t aBitMask = 0;
   };

   struct DDSHeader
   {
      uint32_t size = 0;
      DDSFlags::Enum flags = DDSFlags::None;
      uint32_t height = 0;
      uint32_t width = 0;
      uint32_t pitchOrLinearSize = 0;
      uint32_t depth = 0;
      uint32_t mipMapCount = 0;
      std::array<uint32_t, 11> reserved1;
      DDSPixelFormat pixelFormat;
      DDSCaps::Enum caps = DDSCaps::None;
      DDSCaps2::Enum caps2 = DDSCaps2::None;
      uint32_t caps3 = 0;
      uint32_t caps4 = 0;
      uint32_t reserved2 = 0;
   };

   struct DDSHeaderDX10
   {
      DXGIFormat dxgiFormat = DXGI_FORMAT_UNKNOWN;
      D3D10ResourceDimension resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;
      DDSDX10MiscFlag::Enum miscFlag = DDSDX10MiscFlag::None;
      uint32_t arraySize = 0;
      DDSDX10AlphaMode::Enum miscFlags2 = DDSDX10AlphaMode::Unknown;
   };

   vk::Format dxgiToVk(DXGIFormat dxgiFormat)
   {
      switch (dxgiFormat)
      {
      case DXGI_FORMAT_UNKNOWN:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_R32G32B32A32_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R32G32B32A32_FLOAT:
         return vk::Format::eR32G32B32A32Sfloat;
      case DXGI_FORMAT_R32G32B32A32_UINT:
         return vk::Format::eR32G32B32A32Uint;
      case DXGI_FORMAT_R32G32B32A32_SINT:
         return vk::Format::eR32G32B32A32Sint;

      case DXGI_FORMAT_R32G32B32_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R32G32B32_FLOAT:
         return vk::Format::eR32G32B32Sfloat;
      case DXGI_FORMAT_R32G32B32_UINT:
         return vk::Format::eR32G32B32Uint;
      case DXGI_FORMAT_R32G32B32_SINT:
         return vk::Format::eR32G32B32Sint;

      case DXGI_FORMAT_R16G16B16A16_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
         return vk::Format::eR16G16B16A16Sfloat;
      case DXGI_FORMAT_R16G16B16A16_UNORM:
         return vk::Format::eR16G16B16A16Unorm;
      case DXGI_FORMAT_R16G16B16A16_UINT:
         return vk::Format::eR16G16B16A16Uint;
      case DXGI_FORMAT_R16G16B16A16_SNORM:
         return vk::Format::eR16G16B16A16Snorm;
      case DXGI_FORMAT_R16G16B16A16_SINT:
         return vk::Format::eR16G16B16A16Sint;

      case DXGI_FORMAT_R32G32_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R32G32_FLOAT:
         return vk::Format::eR32G32Sfloat;
      case DXGI_FORMAT_R32G32_UINT:
         return vk::Format::eR32G32Uint;
      case DXGI_FORMAT_R32G32_SINT:
         return vk::Format::eR32G32Sint;

      case DXGI_FORMAT_R32G8X24_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
         return vk::Format::eD32SfloatS8Uint;
      case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_R10G10B10A2_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R10G10B10A2_UNORM:
         return vk::Format::eA2R10G10B10UnormPack32;
      case DXGI_FORMAT_R10G10B10A2_UINT:
         return vk::Format::eA2R10G10B10UintPack32;

      case DXGI_FORMAT_R11G11B10_FLOAT:
         return vk::Format::eB10G11R11UfloatPack32;

      case DXGI_FORMAT_R8G8B8A8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R8G8B8A8_UNORM:
         return vk::Format::eR8G8B8A8Unorm;
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
         return vk::Format::eR8G8B8A8Srgb;
      case DXGI_FORMAT_R8G8B8A8_UINT:
         return vk::Format::eR8G8B8A8Uint;
      case DXGI_FORMAT_R8G8B8A8_SNORM:
         return vk::Format::eR8G8B8A8Snorm;
      case DXGI_FORMAT_R8G8B8A8_SINT:
         return vk::Format::eR8G8B8A8Sint;

      case DXGI_FORMAT_R16G16_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R16G16_FLOAT:
         return vk::Format::eR16G16Sfloat;
      case DXGI_FORMAT_R16G16_UNORM:
         return vk::Format::eR16G16Unorm;
      case DXGI_FORMAT_R16G16_UINT:
         return vk::Format::eR16G16Uint;
      case DXGI_FORMAT_R16G16_SNORM:
         return vk::Format::eR16G16Snorm;
      case DXGI_FORMAT_R16G16_SINT:
         return vk::Format::eR16G16Sint;

      case DXGI_FORMAT_R32_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_D32_FLOAT:
         return vk::Format::eD32Sfloat;
      case DXGI_FORMAT_R32_FLOAT:
         return vk::Format::eR32Sfloat;
      case DXGI_FORMAT_R32_UINT:
         return vk::Format::eR32Uint;
      case DXGI_FORMAT_R32_SINT:
         return vk::Format::eR32Sint;

      case DXGI_FORMAT_R24G8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
         return vk::Format::eD24UnormS8Uint;
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_R8G8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R8G8_UNORM:
         return vk::Format::eR8G8Unorm;
      case DXGI_FORMAT_R8G8_UINT:
         return vk::Format::eR8G8Uint;
      case DXGI_FORMAT_R8G8_SNORM:
         return vk::Format::eR8G8Snorm;
      case DXGI_FORMAT_R8G8_SINT:
         return vk::Format::eR8G8Sint;

      case DXGI_FORMAT_R16_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R16_FLOAT:
         return vk::Format::eR16Sfloat;
      case DXGI_FORMAT_D16_UNORM:
         return vk::Format::eD16Unorm;
      case DXGI_FORMAT_R16_UNORM:
         return vk::Format::eR16Unorm;
      case DXGI_FORMAT_R16_UINT:
         return vk::Format::eR16Uint;
      case DXGI_FORMAT_R16_SNORM:
         return vk::Format::eR16Snorm;
      case DXGI_FORMAT_R16_SINT:
         return vk::Format::eR16Sint;

      case DXGI_FORMAT_R8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R8_UNORM:
         return vk::Format::eR8Unorm;
      case DXGI_FORMAT_R8_UINT:
         return vk::Format::eR8Uint;
      case DXGI_FORMAT_R8_SNORM:
         return vk::Format::eR8Snorm;
      case DXGI_FORMAT_R8_SINT:
         return vk::Format::eR8Sint;
      case DXGI_FORMAT_A8_UNORM:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_R1_UNORM:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
         return vk::Format::eE5B9G9R9UfloatPack32;

      case DXGI_FORMAT_R8G8_B8G8_UNORM:
         return vk::Format::eB8G8R8G8422Unorm;
      case DXGI_FORMAT_G8R8_G8B8_UNORM:
         return vk::Format::eG8B8G8R8422Unorm;

      case DXGI_FORMAT_BC1_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC1_UNORM:
         return vk::Format::eBc1RgbaUnormBlock;
      case DXGI_FORMAT_BC1_UNORM_SRGB:
         return vk::Format::eBc1RgbaSrgbBlock;

      case DXGI_FORMAT_BC2_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC2_UNORM:
         return vk::Format::eBc2UnormBlock;
      case DXGI_FORMAT_BC2_UNORM_SRGB:
         return vk::Format::eBc2SrgbBlock;

      case DXGI_FORMAT_BC3_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC3_UNORM:
         return vk::Format::eBc3UnormBlock;
      case DXGI_FORMAT_BC3_UNORM_SRGB:
         return vk::Format::eBc3SrgbBlock;

      case DXGI_FORMAT_BC4_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC4_UNORM:
         return vk::Format::eBc4UnormBlock;
      case DXGI_FORMAT_BC4_SNORM:
         return vk::Format::eBc4SnormBlock;

      case DXGI_FORMAT_BC5_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC5_UNORM:
         return vk::Format::eBc5UnormBlock;
      case DXGI_FORMAT_BC5_SNORM:
         return vk::Format::eBc5SnormBlock;

      case DXGI_FORMAT_B5G6R5_UNORM:
         return vk::Format::eB5G6R5UnormPack16;
      case DXGI_FORMAT_B5G5R5A1_UNORM:
         return vk::Format::eB5G5R5A1UnormPack16;
      case DXGI_FORMAT_B8G8R8A8_UNORM:
         return vk::Format::eB8G8R8A8Unorm;
      case DXGI_FORMAT_B8G8R8X8_UNORM:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_B8G8R8A8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
         return vk::Format::eB8G8R8A8Srgb;
      case DXGI_FORMAT_B8G8R8X8_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_BC6H_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC6H_UF16:
         return vk::Format::eBc6HUfloatBlock;
      case DXGI_FORMAT_BC6H_SF16:
         return vk::Format::eBc6HSfloatBlock;

      case DXGI_FORMAT_BC7_TYPELESS:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_BC7_UNORM:
         return vk::Format::eBc7UnormBlock;
      case DXGI_FORMAT_BC7_UNORM_SRGB:
         return vk::Format::eBc7SrgbBlock;

      case DXGI_FORMAT_AYUV:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_Y410:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_Y416:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_NV12:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_P010:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_P016:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_420_OPAQUE:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_YUY2:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_Y210:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_Y216:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_NV11:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_AI44:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_IA44:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_P8:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_A8P8:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_B4G4R4A4_UNORM:
         return vk::Format::eB4G4R4A4UnormPack16;

      case DXGI_FORMAT_P208:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_V208:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_V408:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:
         return vk::Format::eUndefined;
      case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE:
         return vk::Format::eUndefined;

      case DXGI_FORMAT_FORCE_UINT:
         return vk::Format::eUndefined;

      default:
         return vk::Format::eUndefined;
      }
   }

   bool matchesBitmask(const DDSPixelFormat& ddsFormat, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
   {
      return ddsFormat.rBitMask == r && ddsFormat.gBitMask == g && ddsFormat.bBitMask == b && ddsFormat.aBitMask == a;
   }

   vk::Format ddsToVkFormat(const DDSPixelFormat& ddsFormat, const DDSHeaderDX10& headerDx10, bool sRGBHint)
   {
      if (ddsFormat.flags & DDSPixelFormatFlags::RGB)
      {
         switch (ddsFormat.rgbBitCount)
         {
         case 16:
            if (matchesBitmask(ddsFormat, 0x7c00, 0x03e0, 0x001f, 0x8000))
            {
               return vk::Format::eB5G5R5A1UnormPack16;
            }
            if (matchesBitmask(ddsFormat, 0xf800, 0x07e0, 0x001f, 0x0000))
            {
               return vk::Format::eB5G6R5UnormPack16;
            }
            if (matchesBitmask(ddsFormat, 0x0f00, 0x00f0, 0x000f, 0xf000))
            {
               return vk::Format::eB4G4R4A4UnormPack16;
            }
            break;
         case 32:
            if (matchesBitmask(ddsFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
               return vk::Format::eR8G8B8A8Unorm;
            }
            if (matchesBitmask(ddsFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
            {
               return vk::Format::eB8G8R8A8Unorm;
            }
            if (matchesBitmask(ddsFormat, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000))
            {
               return vk::Format::eA2R10G10B10UnormPack32;
            }
            if (matchesBitmask(ddsFormat, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
            {
               return vk::Format::eR32Sfloat;
            }
            break;
         default:
            break;
         }
      }
      else if (ddsFormat.flags & DDSPixelFormatFlags::Luminance)
      {
         switch (ddsFormat.rgbBitCount)
         {
         case 8:
            if (matchesBitmask(ddsFormat, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
            {
               return vk::Format::eR8Unorm;
            }
            break;
         case 16:
            if (matchesBitmask(ddsFormat, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
            {
               return vk::Format::eR16Unorm;
            }
            if (matchesBitmask(ddsFormat, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
               return vk::Format::eR8G8Unorm;
            }
            break;
         default:
            break;
         }
      }
      else if (ddsFormat.flags & DDSPixelFormatFlags::Alpha)
      {
         return vk::Format::eUndefined;
      }
      else if (ddsFormat.flags & DDSPixelFormatFlags::FourCC)
      {
         switch (ddsFormat.fourCC)
         {
         case DDSFourCC::DXT1:
            return sRGBHint ? vk::Format::eBc1RgbSrgbBlock : vk::Format::eBc1RgbUnormBlock;
         case DDSFourCC::DXT2:
         case DDSFourCC::DXT3:
            return sRGBHint ? vk::Format::eBc2SrgbBlock : vk::Format::eBc2UnormBlock;
         case DDSFourCC::DXT4:
         case DDSFourCC::DXT5:
            return sRGBHint ? vk::Format::eBc3SrgbBlock : vk::Format::eBc3UnormBlock;
         case DDSFourCC::ATI1:
            return vk::Format::eBc4UnormBlock;
         case DDSFourCC::ATI2:
            return vk::Format::eBc5UnormBlock;
         case DDSFourCC::BC4U:
            return vk::Format::eBc4UnormBlock;
         case DDSFourCC::BC4S:
            return vk::Format::eBc4SnormBlock;
         case DDSFourCC::BC5U:
            return vk::Format::eBc5UnormBlock;
         case DDSFourCC::BC5S:
            return vk::Format::eBc5SnormBlock;
         case DDSFourCC::RGBG:
            return vk::Format::eB8G8R8G8422Unorm;
         case DDSFourCC::GRGB:
            return vk::Format::eG8B8G8R8422Unorm;
         case DDSFourCC::DX10:
            return dxgiToVk(headerDx10.dxgiFormat);
         default:
            return vk::Format::eUndefined;
         }
      }

      return vk::Format::eUndefined;
   }

   vk::ImageType determineImageType(const DDSHeader& header, const DDSHeaderDX10& headerDx10)
   {
      switch (headerDx10.resourceDimension)
      {
      case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
         return vk::ImageType::e1D;
      case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
         return vk::ImageType::e2D;
      case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
         return vk::ImageType::e3D;
      default:
         if (header.flags & DDSFlags::Depth)
         {
            return vk::ImageType::e3D;
         }
         return vk::ImageType::e2D;
      }
   }

   uint32_t determineLayerCount(const DDSHeader& header, const DDSHeaderDX10& headerDx10)
   {
      if (headerDx10.arraySize > 0)
      {
         return headerDx10.arraySize * (headerDx10.miscFlag & DDSDX10MiscFlag::TextureCube) ? 6 : 1;
      }

      uint32_t allCubemapFaces = DDSCaps2::CubemapPositiveX | DDSCaps2::CubemapNegativeX | DDSCaps2::CubemapPositiveY | DDSCaps2::CubemapNegativeY | DDSCaps2::CubemapPositiveZ | DDSCaps2::CubemapNegativeZ;
      if ((header.caps2 & DDSCaps2::Cubemap) && (header.caps2 & allCubemapFaces) == allCubemapFaces)
      {
         return 6;
      }

      return 1;
   }

   uint32_t computeImageDataSize(vk::Format format, uint32_t width, uint32_t height, uint32_t depth)
   {
      uint32_t bytesPerBlock = FormatHelpers::bytesPerBlock(format);
      if (bytesPerBlock > 0)
      {
         uint32_t numBlocksWide = std::max(1u, (width + 3) / 4);
         uint32_t numBlocksHigh = std::max(1u, (height + 3) / 4);
         uint32_t numBlocksDeep = std::max(1u, (depth + 3) / 4);
         uint32_t numBlocks = numBlocksWide * numBlocksHigh * numBlocksDeep;
         return numBlocks * bytesPerBlock;
      }

      if (format == vk::Format::eB8G8R8G8422Unorm || format == vk::Format::eG8B8G8R8422Unorm)
      {
         uint32_t bytesPerRow = ((width + 1) / 2) * 4;
         uint32_t numRows = height * depth;
         return numRows * bytesPerRow;
      }

      uint32_t bitsPerPixel = FormatHelpers::bitsPerPixel(format);
      uint32_t bytesPerRow = (width * bitsPerPixel + 7) / 8;
      uint32_t numRows = height * depth;
      return numRows * bytesPerRow;
   }

   class DDSImage : public Image
   {
   public:
      DDSImage(const ImageProperties& imageProperties, std::vector<uint8_t> fileData, std::size_t textureDataOffset, std::size_t textureDataSize, std::vector<MipInfo> mipInfo, uint32_t mipMapCount)
         : Image(imageProperties)
         , data(std::move(fileData))
         , textureOffset(textureDataOffset)
         , textureSize(textureDataSize)
         , mips(std::move(mipInfo))
         , mipsPerLayer(mipMapCount)
      {
      }

      TextureData getTextureData() const final
      {
         TextureData textureData;
         textureData.bytes = std::span<const uint8_t>(data.data() + textureOffset, textureSize);
         textureData.mips = std::span<const MipInfo>(mips);
         textureData.mipsPerLayer = mipsPerLayer;
         return textureData;
      }

   private:
      std::vector<uint8_t> data;
      std::size_t textureOffset = 0;
      std::size_t textureSize = 0;
      std::vector<MipInfo> mips;
      uint32_t mipsPerLayer = 0;
   };
}

namespace DDSImageLoader
{
   std::unique_ptr<Image> loadImage(std::vector<uint8_t> fileData, bool sRGBHint)
   {
      if (fileData.size() < sizeof(uint32_t) + sizeof(DDSHeader))
      {
         return nullptr;
      }

      std::size_t offset = 0;

      uint32_t magic = 0;
      std::memcpy(&magic, fileData.data() + offset, sizeof(uint32_t));
      offset += sizeof(uint32_t);

      DDSHeader header;
      std::memcpy(&header, fileData.data() + offset, sizeof(DDSHeader));
      offset += sizeof(DDSHeader);

      constexpr uint32_t kDDSMagic = fourCC("DDS ");
      if (magic != kDDSMagic || header.size != sizeof(DDSHeader))
      {
         return nullptr;
      }

      DDSHeaderDX10 headerDX10;
      if ((header.pixelFormat.flags & DDSPixelFormatFlags::FourCC) && header.pixelFormat.fourCC == DDSFourCC::DX10 && fileData.size() >= offset + sizeof(DDSHeaderDX10))
      {
         std::memcpy(&headerDX10, fileData.data() + offset, sizeof(DDSHeaderDX10));
         offset += sizeof(DDSHeaderDX10);
      }

      ImageProperties properties;
      properties.format = ddsToVkFormat(header.pixelFormat, headerDX10, sRGBHint);
      if (properties.format == vk::Format::eUndefined)
      {
         return nullptr;
      }
      properties.type = determineImageType(header, headerDX10);
      if (header.flags & DDSFlags::Width)
      {
         properties.width = header.width;
      }
      if (header.flags & DDSFlags::Height)
      {
         properties.height = header.height;
      }
      if (header.flags & DDSFlags::Depth)
      {
         properties.depth = header.depth;
      }
      properties.layers = determineLayerCount(header, headerDX10);
      properties.hasAlpha = FormatHelpers::hasAlpha(properties.format);
      properties.cubeCompatible = (header.caps2 & DDSCaps2::Cubemap) || (headerDX10.miscFlag & DDSDX10MiscFlag::TextureCube);

      uint32_t mipMapCount = 1;
      if (header.flags & DDSFlags::MipMapCount)
      {
         mipMapCount = header.mipMapCount;
      }

      std::size_t textureDataOffset = offset;
      std::size_t textureDataSize = 0;

      std::vector<MipInfo> mips;
      mips.reserve(mipMapCount * properties.layers);
      for (uint32_t layer = 0; layer < properties.layers; ++layer)
      {
         uint32_t mipWidth = properties.width;
         uint32_t mipHeight = properties.height;
         uint32_t mipDepth = properties.depth;
         for (uint32_t i = 0; i < mipMapCount; ++i)
         {
            uint32_t dataSize = computeImageDataSize(properties.format, mipWidth, mipHeight, mipDepth);
            textureDataSize += dataSize;
            if (fileData.size() < offset + dataSize)
            {
               return nullptr;
            }

            MipInfo mipInfo;
            mipInfo.extent = vk::Extent3D(mipWidth, mipHeight, mipDepth);
            mipInfo.bufferOffset = static_cast<uint32_t>(offset - textureDataOffset);
            mips.push_back(mipInfo);

            offset += dataSize;
            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
            mipDepth = std::max(1u, mipDepth / 2);
         }
      }

      return std::make_unique<DDSImage>(properties, std::move(fileData), textureDataOffset, textureDataSize, std::move(mips), mipMapCount);
   }
}
