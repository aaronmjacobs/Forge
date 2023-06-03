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

enum class TonemappingAlgorithm
{
   None,
   Curve,
   Reinhard,
   TonyMcMapface
};

struct RenderSettings
{
   vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
   RenderQuality ssaoQuality = RenderQuality::Medium;
   RenderQuality bloomQuality = RenderQuality::High;
   bool presentHDR = false;
   TonemappingAlgorithm tonemappingAlgorithm = TonemappingAlgorithm::Curve;

   bool operator==(const RenderSettings& other) const = default;

   static const char* getQualityString(RenderQuality quality);
};
