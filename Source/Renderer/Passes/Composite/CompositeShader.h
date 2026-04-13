#pragma once

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include <vector>

enum class CompositeMode
{
   Passthrough = 0,
   LinearToSrgb = 1,
   SrgbToLinear = 2
};

struct CompositeShaderConstants
{
   CompositeMode mode = CompositeMode::Passthrough;

   static void registerMembers(ShaderPermutationManager<CompositeShaderConstants>& permutationManager);

   bool operator==(const CompositeShaderConstants& other) const = default;
};

class CompositeDescriptorSet : public TypedDescriptorSet<CompositeDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class CompositeShader : public ParameterizedShader<CompositeShaderConstants, CompositeDescriptorSet>
{
public:
   CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
};
