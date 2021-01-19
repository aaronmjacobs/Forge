#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceManager.h"

#include <vector>

class Material;
class Texture;
class View;

class SimpleShader : public GraphicsResource
{
public:
   SimpleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, const View& view, vk::PipelineLayout pipelineLayout, const Material& material);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTexture) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfo;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithTexture;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithoutTexture;
};
