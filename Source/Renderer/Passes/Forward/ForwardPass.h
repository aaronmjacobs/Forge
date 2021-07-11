#pragma once

#include "Graphics/RenderPass.h"

#include "Renderer/Passes/Forward/ForwardLighting.h"

#include <memory>

class Material;
class ResourceManager;
class ForwardShader;
struct MeshSection;
struct SceneRenderInfo;

class ForwardPass : public RenderPass
{
public:
   ForwardPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager);
   ~ForwardPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

protected:
   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

private:
   template<bool translucency>
   void renderMeshes(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

   vk::Pipeline selectPipeline(const MeshSection& meshSection, const Material& material) const;

   std::unique_ptr<ForwardShader> forwardShader;
   ForwardLighting lighting;
};
