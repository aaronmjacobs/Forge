#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <vector>

struct TonemapShaderConstants
{
   TonemappingAlgorithm tonemappingAlgorithm = TonemappingAlgorithm::None;
   ColorGamut colorGamut = ColorGamut::Rec709;
   TransferFunction transferFunction = TransferFunction::Linear;
   VkBool32 outputHDR = false;
   VkBool32 withBloom = false;
   VkBool32 withUI = false;
   VkBool32 showTestPattern = false;

   static void registerMembers(ShaderPermutationManager<TonemapShaderConstants>& permutationManager);

   bool operator==(const TonemapShaderConstants& other) const = default;
};

class TonemapDescriptorSet : public TypedDescriptorSet<TonemapDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class TonemapShader : public ParameterizedShader<TonemapShaderConstants, TonemapDescriptorSet>
{
public:
   TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
};
