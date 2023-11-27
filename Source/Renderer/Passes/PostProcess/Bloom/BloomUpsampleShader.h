#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/RenderSettings.h"

#include <vector>

class BloomUpsampleDescriptorSet : public TypedDescriptorSet<BloomUpsampleDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class BloomUpsampleShader : public ShaderWithDescriptors<BloomUpsampleDescriptorSet>
{
public:
   BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(RenderQuality quality, bool horizontal) const;
};
