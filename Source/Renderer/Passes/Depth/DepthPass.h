#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <memory>

class DepthShader;
class ResourceManager;
struct SceneRenderInfo;

class DepthPass : public SceneRenderPass<DepthPass>
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, bool shadowPass = false);
   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle);

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

protected:
   friend class SceneRenderPass<DepthPass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   vk::Pipeline selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const;

private:
   std::unique_ptr<DepthShader> depthShader;
   bool isShadowPass = false;
};
