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

   void render(vk::CommandBuffer commandBuffer, Texture& uiTexture);

   void updateFramebuffer(const Texture& uiTexture);

protected:
   void postUpdateAttachmentFormats() override;

private:
   void initializeRenderPass();
   void terminateRenderPass();

   void initializeFramebuffer(const Texture& uiTexture);
   void terminateFramebuffer();

   void initializeImgui();
   void terminateImgui();

   vk::RenderPass renderPass;
   vk::Framebuffer framebuffer;

   vk::DescriptorPool descriptorPool;
   bool imguiInitialized = false;
};
