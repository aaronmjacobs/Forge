#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"
#include "Graphics/UniformData.h"

#include "Resources/ShaderModuleResourceManager.h"

#include <vector>

class Texture;

class SimpleShader : public GraphicsResource
{
public:
   SimpleShader(ShaderModuleResourceManager& shaderModuleResourceManager, const VulkanContext& context);

   ~SimpleShader();

   void allocateDescriptorSets(vk::DescriptorPool descriptorPool, uint32_t numSwapchainImages);
   void clearDescriptorSets();

   void updateDescriptorSets(const VulkanContext& context, uint32_t numSwapchainImages, const UniformBuffer<ViewUniformData>& viewUniformBuffer, const UniformBuffer<MeshUniformData>& meshUniformBuffer, const Texture& texture, vk::Sampler sampler);
   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t swapchainIndex);

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
