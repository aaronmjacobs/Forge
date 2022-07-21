#pragma once

#include "Graphics/Shader.h"

#include <vector>

class DescriptorSet;
class ForwardLighting;
class Material;
class View;

class ForwardShader : public Shader
{
public:
   static uint32_t getPermutationIndex(bool withTextures, bool withBlending);

   static std::array<vk::DescriptorSetLayoutBinding, 2> getBindings();
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const DescriptorSet& descriptorSet, const ForwardLighting& lighting, const Material& material);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTextures, bool withBlending) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
