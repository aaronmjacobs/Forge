#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <vector>

class TonemapDescriptorSet : public TypedDescriptorSet<TonemapDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class TonemapShader : public ShaderWithDescriptors<TonemapDescriptorSet>
{
public:
   TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(TonemappingAlgorithm tonemappingAlgorithm, bool outputHDR, bool withBloom, bool withUI, bool showTestPattern) const;
};
