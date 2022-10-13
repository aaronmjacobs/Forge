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
   void initializeRenderPass(vk::Format format, vk::SampleCountFlagBits sampleCount);
   void terminateRenderPass();

   void initializeFramebuffer(const Texture& uiTexture);
   void terminateFramebuffer();

   void initializeImgui();
   void terminateImgui();

   vk::RenderPass renderPass;
   vk::Framebuffer framebuffer;

   vk::DescriptorPool descriptorPool;
   bool imguiInitialized = false;

   vk::Format cachedFormat = vk::Format::eUndefined;
   vk::SampleCountFlagBits cachedSampleCount = vk::SampleCountFlagBits::e1;
};
