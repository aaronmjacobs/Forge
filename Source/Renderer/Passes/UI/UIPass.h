#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/RenderPass.h"

#include <memory>

class Texture;

class UIPass : public RenderPass
{
public:
   UIPass(const GraphicsContext& graphicsContext);
   ~UIPass();

   void render(vk::CommandBuffer commandBuffer, Texture& outputTexture);

   void onOutputTextureCreated(const Texture& outputTexture);

private:
   void initializeImgui(vk::Format format, vk::SampleCountFlagBits sampleCount);
   void terminateImgui();

   vk::DescriptorPool descriptorPool;

   bool imguiInitialized = false;
   vk::Format cachedFormat = vk::Format::eUndefined;
   vk::SampleCountFlagBits cachedSampleCount = vk::SampleCountFlagBits::e1;
};
