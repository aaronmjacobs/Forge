#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceManager.h"

#include <vector>

class View;

class DepthShader : public GraphicsResource
{
public:
   DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void updateDescriptorSets(const View& view);
   void bindDescriptorSets(vk::CommandBuffer commandBuffer, const View& view, vk::PipelineLayout pipelineLayout);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfo;
};
