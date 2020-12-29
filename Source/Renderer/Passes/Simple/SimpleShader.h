#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/UniformData.h"

#include "Resources/ResourceManager.h"

#include <vector>

class Swapchain;
class Texture;

class SimpleShader : public GraphicsResource
{
public:
   SimpleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   ~SimpleShader();

   void allocateDescriptorSets(vk::DescriptorPool descriptorPool);
   void clearDescriptorSets();

   void updateDescriptorSets(const UniformBuffer<ViewUniformData>& viewUniformBuffer, const Texture& texture, vk::Sampler sampler);
   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout);

   bool areDescriptorSetsAllocated() const
   {
      return !frameSets.empty() || !drawSets.empty();
   }

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTexture) const
   {
      return { vertStageCreateInfo, withTexture ? fragStageCreateInfoWithTexture : fragStageCreateInfoWithoutTexture };
   }

   std::vector<vk::DescriptorSetLayout> getSetLayouts() const
   {
      return { frameLayout, drawLayout };
   }

   std::vector<vk::PushConstantRange> getPushConstantRanges() const;

private:
   vk::PipelineShaderStageCreateInfo vertStageCreateInfo;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithTexture;
   vk::PipelineShaderStageCreateInfo fragStageCreateInfoWithoutTexture;

   std::vector<vk::PipelineShaderStageCreateInfo> stages;

   vk::DescriptorSetLayout frameLayout;
   vk::DescriptorSetLayout drawLayout;

   std::vector<vk::DescriptorSet> frameSets;
   std::vector<vk::DescriptorSet> drawSets;
};
