#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceManager.h"

#include <vector>

class Material;
class View;

class DepthMaskedShader : public GraphicsResource
{
public:
   DepthMaskedShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Material& material);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfo;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfo;
};
