#pragma once

#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <array>
#include <vector>

class DescriptorSet;

class TonemapShader : public Shader
{
public:
   static std::array<vk::DescriptorSetLayoutBinding, 4> getBindings();
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool outputHDR, bool withBloom, bool withUI, TonemappingAlgorithm tonemappingAlgorithm) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
};
