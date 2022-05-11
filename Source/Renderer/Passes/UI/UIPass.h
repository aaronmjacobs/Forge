#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/RenderPass.h"

#include <memory>

class UIPass : public RenderPass
{
public:
   UIPass(const GraphicsContext& graphicsContext);
   ~UIPass();

   void render(vk::CommandBuffer commandBuffer, FramebufferHandle framebufferHandle);

protected:
   void postRenderPassInitialized() override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

private:
   void initializeImgui();
   void terminateImgui();

   vk::DescriptorPool descriptorPool;
   bool imguiInitialized = false;
};
