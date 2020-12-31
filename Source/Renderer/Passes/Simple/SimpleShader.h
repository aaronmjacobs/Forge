#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"

#include "Resources/ResourceManager.h"

#include <vector>

class Swapchain;
class Texture;
class View;

class SimpleShader : public GraphicsResource
{
public:
   SimpleShader(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager);

   ~SimpleShader();

   void updateDescriptorSets(const View& view, const Texture& texture, vk::Sampler sampler);
   void bindDescriptorSets(vk::CommandBuffer commandBuffer, const View& view, vk::PipelineLayout pipelineLayout);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTexture) const
   {
      return { vertStageCreateInfo, withTexture ? fragStageCreateInfoWithTexture : fragStageCreateInfoWithoutTexture };
   }

   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfo;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithTexture;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithoutTexture;

   std::vector<vk::PipelineShaderStageCreateInfo> stages;

   DescriptorSet descriptorSet;
};
