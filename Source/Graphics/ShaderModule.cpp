#include "Graphics/ShaderModule.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"

#include <utility>

ShaderModule::ShaderModule(const GraphicsContext& graphicsContext, std::span<const uint8_t> code)
   : GraphicsResource(graphicsContext)
{
   ASSERT(code.size() > 0);

   vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
      .setCodeSize(code.size())
      .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

   shaderModule = device.createShaderModule(createInfo);
   NAME_CHILD(shaderModule, "Shader Module");
}

ShaderModule::~ShaderModule()
{
   ASSERT(shaderModule);
   context.delayedDestroy(std::move(shaderModule));
}
