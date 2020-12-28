#pragma once

#include "Graphics/GraphicsResource.h"

#include <cstdint>
#include <vector>

class ShaderModule : public GraphicsResource
{
public:
   ShaderModule(const GraphicsContext& graphicsContext, const std::vector<uint8_t>& code);

   ~ShaderModule();

   vk::ShaderModule getShaderModule() const
   {
      return shaderModule;
   }

private:
   vk::ShaderModule shaderModule;
};
