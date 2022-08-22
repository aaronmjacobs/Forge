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

struct RenderSettings
{
   vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
   RenderQuality ssaoQuality = RenderQuality::Medium;
   RenderQuality bloomQuality = RenderQuality::High;
   bool presentHDR = false;

   bool operator==(const RenderSettings& other) const = default;

   static const char* getQualityString(RenderQuality quality);
};
