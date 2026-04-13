#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/View.h"

#include <vector>

struct ForwardShaderConstants
{
   VkBool32 withTextures = false;
   VkBool32 withBlending = false;

   static void registerMembers(ShaderPermutationManager<ForwardShaderConstants>& permutationManager);

   bool operator==(const ForwardShaderConstants& other) const = default;
};

class ForwardDescriptorSet : public TypedDescriptorSet<ForwardDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class ForwardShader : public ParameterizedShader<ForwardShaderConstants, ViewDescriptorSet, ForwardDescriptorSet, ForwardLightingDescriptorSet, PhysicallyBasedMaterialDescriptorSet>
{
public:
   ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
