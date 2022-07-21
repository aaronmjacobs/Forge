#pragma once

#include "Graphics/Shader.h"

#include <array>
#include <vector>

class DescriptorSet;
class View;

class SSAOBlurShader : public Shader
{
public:
   static std::array<vk::DescriptorSetLayoutBinding, 2> getBindings();
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   SSAOBlurShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool horizontal) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
};
