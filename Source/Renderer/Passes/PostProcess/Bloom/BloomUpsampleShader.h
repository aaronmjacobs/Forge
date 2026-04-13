#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <vector>

struct BloomUpsampleShaderConstants
{
   RenderQuality quality = RenderQuality::High;
   VkBool32 horizontal = false;

   static void registerMembers(ShaderPermutationManager<BloomUpsampleShaderConstants>& permutationManager);

   bool operator==(const BloomUpsampleShaderConstants& other) const = default;
};

class BloomUpsampleDescriptorSet : public TypedDescriptorSet<BloomUpsampleDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class BloomUpsampleShader : public ParameterizedShader<BloomUpsampleShaderConstants, BloomUpsampleDescriptorSet>
{
public:
   BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
};
