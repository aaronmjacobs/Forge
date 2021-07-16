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

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

private:
   vk::ShaderModule shaderModule;
};
