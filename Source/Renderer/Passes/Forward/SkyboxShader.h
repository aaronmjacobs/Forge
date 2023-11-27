#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

class SkyboxDescriptorSet : public TypedDescriptorSet<SkyboxDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class SkyboxShader : public ShaderWithDescriptors<ViewDescriptorSet, SkyboxDescriptorSet>
{
public:
   SkyboxShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
};
