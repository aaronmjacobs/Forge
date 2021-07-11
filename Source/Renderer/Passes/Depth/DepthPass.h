#pragma once

#include "Graphics/RenderPass.h"

#include <memory>

class DepthShader;
class ResourceManager;
struct SceneRenderInfo;

class DepthPass : public RenderPass
{
public:
   DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
   ~DepthPass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo);

protected:
   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

private:
   std::unique_ptr<DepthShader> depthShader;
};
