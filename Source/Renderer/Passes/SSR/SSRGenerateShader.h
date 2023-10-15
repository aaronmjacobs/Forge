#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

class SSRGenerateDescriptorSet : public TypedDescriptorSet<SSRGenerateDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class SSRGenerateShader : public ShaderWithDescriptors<ViewDescriptorSet, SSRGenerateDescriptorSet>
{
public:
   SSRGenerateShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
};
