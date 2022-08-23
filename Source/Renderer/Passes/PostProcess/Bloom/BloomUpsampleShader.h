#pragma once

#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <array>
#include <vector>

class DescriptorSet;

class BloomUpsampleShader : public Shader
{
public:
   static std::array<vk::DescriptorSetLayoutBinding, 3> getBindings();
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(RenderQuality quality, bool horizontal) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
};
