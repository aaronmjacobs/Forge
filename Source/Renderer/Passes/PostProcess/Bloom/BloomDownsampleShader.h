#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <vector>

struct BloomDownsampleShaderConstants
{
   RenderQuality quality = RenderQuality::High;

   static void registerMembers(ShaderPermutationManager<BloomDownsampleShaderConstants>& permutationManager);

   bool operator==(const BloomDownsampleShaderConstants& other) const = default;
};

class BloomDownsampleDescriptorSet : public TypedDescriptorSet<BloomDownsampleDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class BloomDownsampleShader : public ParameterizedShader<BloomDownsampleShaderConstants, BloomDownsampleDescriptorSet>
{
public:
   BloomDownsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
};
