#pragma once

#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

class DepthShader : public ShaderWithDescriptors<ViewDescriptorSet>
{
public:
   DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;
   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
