#pragma once

#include "Graphics/GraphicsResource.h"

#include <cstdint>
#include <span>

class ShaderModule : public GraphicsResource
{
public:
   ShaderModule(const GraphicsContext& graphicsContext, std::span<const uint8_t> code);
   ~ShaderModule();

   vk::ShaderModule getShaderModule() const
   {
      return shaderModule;
   }

private:
   vk::ShaderModule shaderModule;
};
