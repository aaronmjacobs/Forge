#pragma once

#include "Renderer/Passes/Forward/ForwardLighting.h"
#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class ResourceManager;
class ForwardShader;

class ForwardPass : public SceneRenderPass<ForwardPass>
{
public:
   ForwardPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager);
   ~ForwardPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

protected:
   friend class SceneRenderPass<ForwardPass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;
   void postUpdateAttachments() override;

   void renderMesh(vk::CommandBuffer commandBuffer, const View& view, const Mesh& mesh, uint32_t section, const Material& material);
   vk::Pipeline selectPipeline(const MeshSection& meshSection, const Material& material) const;

private:
   std::unique_ptr<ForwardShader> forwardShader;
   ForwardLighting lighting;
};
