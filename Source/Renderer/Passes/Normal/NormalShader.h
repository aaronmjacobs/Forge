#pragma once

#include "Graphics/Shader.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/View.h"

#include <vector>

class NormalShader : public ShaderWithDescriptors<ViewDescriptorSet, PhysicallyBasedMaterialDescriptorSet>
{
public:
   NormalShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages(bool withTextures, bool masked) const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
