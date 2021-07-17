#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceManager.h"

#include <vector>

class DescriptorSet;

class TonemapShader : public GraphicsResource
{
public:
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfo;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfo;
};
