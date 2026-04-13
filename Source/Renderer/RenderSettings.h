#pragma once

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

#if FORGE_PLATFORM_MACOS
constexpr float kDefaultPeakBrightness = 1600.0f;
#else
constexpr float kDefaultPeakBrightness = 1000.0f;
#endif

struct TonemapSettings
{
   TonemappingAlgorithm algorithm = TonemappingAlgorithm::DoubleFine;

   bool showTestPattern = false;

   float bloomStrength = 0.05f;
   float peakBrightness = kDefaultPeakBrightness;

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
   bool limitFrameRate = true;
   bool presentHDR = false;
   TonemapSettings tonemapSettings;

   bool operator==(const RenderSettings& other) const = default;

   static const char* getQualityString(RenderQuality quality);
};
