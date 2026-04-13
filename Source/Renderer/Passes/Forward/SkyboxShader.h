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

class SkyboxShader : public ParameterizedShader<void, ViewDescriptorSet, SkyboxDescriptorSet>
{
public:
   SkyboxShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);
};
