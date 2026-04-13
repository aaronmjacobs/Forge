#pragma once

#include "Graphics/Shader.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/View.h"

#include <vector>

struct NormalShaderConstants
{
   VkBool32 withTextures = false;
   VkBool32 masked = false;

   static void registerMembers(ShaderPermutationManager<NormalShaderConstants>& permutationManager);

   bool operator==(const NormalShaderConstants& other) const = default;
};

class NormalShader : public ParameterizedShader<NormalShaderConstants, ViewDescriptorSet, PhysicallyBasedMaterialDescriptorSet>
{
public:
   NormalShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
