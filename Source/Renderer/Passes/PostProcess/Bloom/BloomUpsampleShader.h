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

   static uint32_t getPermutationIndex(bool horizontal, RenderQuality quality);

   BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool horizontal, RenderQuality quality) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
};
