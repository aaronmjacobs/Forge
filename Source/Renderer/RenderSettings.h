#pragma once

#include "Graphics/Swapchain.h"
#include "Graphics/Vulkan.h"

struct RenderCapabilities
{
   bool canPresentHDR = false;
};

enum class RenderQuality
{
   Disabled,
   Low,
   Medium,
   High
};

enum class ColorGamut
{
   Rec709,
   Rec2020,
   P3
};

enum class TransferFunction
{
   Linear,
   sRGB,
   PerceptualQuantizer,
   HybridLogGamma
};

enum class TonemappingAlgorithm
{
   None,
   Curve,
   Reinhard,
   TonyMcMapface,
   DoubleFine
};

#if FORGE_PLATFORM_WINDOWS

// "SDR content brightness" in Windows has a visible range of [0,100], which represents a range of [80,480] nits (step size of 4 nits)
// It defaults to 40, which represents (40 * 4) + 80 = 240 nits
constexpr float kDefaultPaperWhiteNits = 240.0f;

// Most HDR displays are 1000 nits
constexpr float kDefaultPeakBrightnessNits = 1000.0f;

#elif FORGE_PLATFORM_MACOS

// macOS seems to have a fixed paper white of 100 nits
constexpr float kDefaultPaperWhiteNits = 100.0f;

// Apple's Mini LED Macbook Pro laptops go up to 1600 nits
constexpr float kDefaultPeakBrightnessNits = 1600.0f;

#else

// 203 nits is the standard reference level
constexpr float kDefaultPaperWhiteNits = 203.0f;

// Most HDR displays are 1000 nits
constexpr float kDefaultPeakBrightnessNits = 1000.0f;

#endif

struct TonemapSettings
{
   TonemappingAlgorithm algorithm = TonemappingAlgorithm::DoubleFine;

   bool showTestPattern = false;

   float bloomStrength = 0.05f;

   float paperWhiteNits = kDefaultPaperWhiteNits;
   float peakBrightnessNits = kDefaultPeakBrightnessNits;
   bool convertToOutputColorGamut = true;

   float toe = 0.0f;
   float shoulder = 0.5f;
   float hotspot = 0.5f;
   float huePreservation = 1.0f;

   bool operator==(const TonemapSettings& other) const = default;
};

struct RenderSettings
{
   vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
   RenderQuality ssaoQuality = RenderQuality::Medium;
   RenderQuality bloomQuality = RenderQuality::High;
   SwapchainSettings swapchainSettings;
   TonemapSettings tonemapSettings;

   bool operator==(const RenderSettings& other) const = default;

   static const char* getQualityString(RenderQuality quality);
};
