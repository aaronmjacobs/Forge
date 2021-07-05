#pragma once

#include "Graphics/GraphicsResource.h"

#include "Renderer/Passes/Forward/ForwardLighting.h"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <vector>

class Material;
class Mesh;
class ResourceManager;
class ForwardShader;
class Texture;
class View;
struct MeshSection;
struct SceneRenderInfo;

class ForwardRenderPass : public GraphicsResource
{
public:
   ForwardRenderPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture);
   ~ForwardRenderPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

   void onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture);

private:
   void initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture);
   void terminateSwapchainDependentResources();

   template<bool translucency>
   void renderMeshes(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

   vk::Pipeline selectPipeline(const MeshSection& meshSection, const Material& material) const;

   std::unique_ptr<ForwardShader> forwardShader;
   ForwardLighting lighting;

   vk::RenderPass renderPass;

   vk::PipelineLayout pipelineLayout;
   std::array<vk::Pipeline, 4> pipelines;

   std::vector<vk::Framebuffer> framebuffers;
};
