#pragma once

#include "Renderer/Passes/SceneRenderPass.h"

#include <glm/glm.hpp>

#include <memory>

class DistanceShader;
class ResourceManager;
struct SceneRenderInfo;

struct DistanceUniformData
{
   alignas(16) glm::vec4 sourcePosition;
};

class DistancePass : public SceneRenderPass<DistancePass>
{
public:
   DistancePass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
   ~DistancePass();

   void render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo, FramebufferHandle framebufferHandle);

protected:
   friend class SceneRenderPass<DistancePass>;

   void initializePipelines(vk::SampleCountFlagBits sampleCount) override;
   std::vector<vk::SubpassDependency> getSubpassDependencies() const override;

   vk::Pipeline selectPipeline(const View& view, const MeshSection& meshSection, const Material& material) const;

private:
   std::unique_ptr<DistanceShader> distanceShader;
};
