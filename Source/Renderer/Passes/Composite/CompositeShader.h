#pragma once

#include "Core/Enum.h"

#include "Graphics/Shader.h"

#include <array>
#include <vector>

class DescriptorSet;

class CompositeShader : public Shader
{
public:
   enum class Mode : uint32_t
   {
      Passthrough = 0,
      LinearToSrgb = 1,
      SrgbToLinear = 2
   };

   static constexpr uint32_t kNumModes = Enum::cast(Mode::SrgbToLinear) + 1;

   static std::array<vk::DescriptorSetLayoutBinding, 1> getBindings();
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   void bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(Mode mode) const;
   std::vector<vk::DescriptorSetLayout> getSetLayouts() const;
};
