#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DepthShader;
class ResourceManager;
struct SceneRenderInfo;

class DepthPass : public SceneRenderPass<DepthPass>
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

protected:
   friend class SceneRenderPass<DepthPass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;
   void postUpdateAttachments() override;

   vk::Pipeline selectPipeline(const MeshSection& meshSection, const Material& material) const
   {
      return pipelines[0];
   }

private:
   std::unique_ptr<DepthShader> depthShader;
};
