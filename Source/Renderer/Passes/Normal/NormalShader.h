#pragma once

#include "Graphics/Shader.h"

#include <vector>

class Material;
class View;

class NormalShader : public Shader
{
public:
   static uint32_t getPermutationIndex(bool withTextures, bool masked);

   NormalShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Material& material);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTextures, bool masked) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
