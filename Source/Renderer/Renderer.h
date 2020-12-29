#pragma once

#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"
#include "Graphics/UniformData.h"

#include "Resources/ResourceManager.h"

#include <memory>
#include <vector>

class SimpleRenderPass;
class Swapchain;
class Texture;

class Renderer : public GraphicsResource
{
public:
   Renderer(const GraphicsContext& graphicsContext, ResourceManager& resourceManagerRef);

   ~Renderer();

   void render(vk::CommandBuffer commandBuffer);

   void updateUniformBuffers();
   void onSwapchainRecreated();

private:
   ResourceManager& resourceManager;

   vk::DescriptorPool descriptorPool;

   std::unique_ptr<Texture> colorTexture;
   std::unique_ptr<Texture> depthTexture;

   std::unique_ptr<SimpleRenderPass> simpleRenderPass;

   std::unique_ptr<UniformBuffer<ViewUniformData>> viewUniformBuffer;
   std::unique_ptr<UniformBuffer<MeshUniformData>> meshUniformBuffer;

   TextureHandle textureHandle;
   MeshHandle meshHandle;
};
