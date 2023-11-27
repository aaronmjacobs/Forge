#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

class SSAOBlurDescriptorSet : public TypedDescriptorSet<SSAOBlurDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class SSAOBlurShader : public ShaderWithDescriptors<ViewDescriptorSet, SSAOBlurDescriptorSet>
{
public:
   SSAOBlurShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool horizontal) const;
};
