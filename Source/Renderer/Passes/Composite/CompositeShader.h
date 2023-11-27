#pragma once

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include <vector>

class CompositeDescriptorSet : public TypedDescriptorSet<CompositeDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class CompositeShader : public ShaderWithDescriptors<CompositeDescriptorSet>
{
public:
   enum class Mode
   {
      Passthrough = 0,
      LinearToSrgb = 1,
      SrgbToLinear = 2
   };

   static constexpr int kNumModes = Enum::cast(Mode::SrgbToLinear) + 1;

   CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(Mode mode) const;
};
