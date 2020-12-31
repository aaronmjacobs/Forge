#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/UniformData.h"

#include "Resources/ResourceManager.h"

#include <memory>
#include <vector>

class SimpleRenderPass;
class Swapchain;
class Texture;
class View;

class Renderer : public GraphicsResource
{
public:
   Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef);

   ~Renderer();

   void render(vk::CommandBuffer commandBuffer);

   void onSwapchainRecreated();

private:
   ResourceManager& resourceManager;

   vk::DescriptorPool descriptorPool;

   std::unique_ptr<View> view;

   std::unique_ptr<Texture> colorTexture;
   std::unique_ptr<Texture> depthTexture;

   std::unique_ptr<SimpleRenderPass> simpleRenderPass;

   TextureHandle textureHandle;
   MeshHandle meshHandle;
};
