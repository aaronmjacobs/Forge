#include "Graphics/GraphicsResource.h"

#include <vector>

class ForwardLighting;
class Material;
class ResourceManager;
class Texture;
class View;

class ForwardShader : public GraphicsResource
{
public:
   ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const ForwardLighting& lighting, const Material& material);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTextures) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfoWithTextures;
   vk::PipelineShaderStageCreateInfo vertStageCreateInfoWithoutTextures;

   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithTextures;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithoutTextures;
};
