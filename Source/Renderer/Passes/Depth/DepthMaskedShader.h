#pragma once

#include "Graphics/Shader.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/View.h"

#include <vector>

class DepthMaskedShader : public ParameterizedShader<void, ViewDescriptorSet, PhysicallyBasedMaterialDescriptorSet>
{
public:
   DepthMaskedShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
