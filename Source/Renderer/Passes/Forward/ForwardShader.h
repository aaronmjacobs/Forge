#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/Shader.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/View.h"

#include <vector>

class ForwardDescriptorSet : public TypedDescriptorSet<ForwardDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class ForwardShader : public ShaderWithDescriptors<ViewDescriptorSet, ForwardDescriptorSet, ForwardLightingDescriptorSet, PhysicallyBasedMaterialDescriptorSet>
{
public:
   ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTextures, bool withBlending) const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
