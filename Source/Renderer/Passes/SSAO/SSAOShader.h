#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

class SSAODescriptorSet : public TypedDescriptorSet<SSAODescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class SSAOShader : public ShaderWithDescriptors<ViewDescriptorSet, SSAODescriptorSet>
{
public:
   SSAOShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
};
