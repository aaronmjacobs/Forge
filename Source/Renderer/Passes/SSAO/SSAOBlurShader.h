#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

struct SSAOBlurShaderConstants
{
   VkBool32 horizontal = false;

   static void registerMembers(ShaderPermutationManager<SSAOBlurShaderConstants>& permutationManager);

   bool operator==(const SSAOBlurShaderConstants& other) const = default;
};

class SSAOBlurDescriptorSet : public TypedDescriptorSet<SSAOBlurDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class SSAOBlurShader : public ParameterizedShader<SSAOBlurShaderConstants, ViewDescriptorSet, SSAOBlurDescriptorSet>
{
public:
   SSAOBlurShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
};
