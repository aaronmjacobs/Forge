#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <vector>

class BloomDownsampleDescriptorSet : public TypedDescriptorSet<BloomDownsampleDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class BloomDownsampleShader : public ShaderWithDescriptors<BloomDownsampleDescriptorSet>
{
public:
   BloomDownsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(RenderQuality quality) const;
};
