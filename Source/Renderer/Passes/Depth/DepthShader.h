#pragma once

#include "Graphics/Shader.h"

#include "Renderer/View.h"

#include <vector>

class DepthShader : public ParameterizedShader<void, ViewDescriptorSet>
{
public:
   DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager);

   std::vector<vk::PushConstantRange> getPushConstantRanges() const;
};
