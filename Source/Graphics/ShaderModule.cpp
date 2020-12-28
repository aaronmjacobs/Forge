#include "Graphics/ShaderModule.h"

#include "Core/Assert.h"

ShaderModule::ShaderModule(const GraphicsContext& graphicsContext, const std::vector<uint8_t>& code)
   : GraphicsResource(graphicsContext)
{
   ASSERT(code.size() > 0);

   vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
      .setCodeSize(code.size())
      .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

   shaderModule = device.createShaderModule(createInfo);
}

ShaderModule::~ShaderModule()
{
   ASSERT(shaderModule);
   device.destroyShaderModule(shaderModule);
}
