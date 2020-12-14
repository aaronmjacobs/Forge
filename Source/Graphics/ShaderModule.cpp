#include "Graphics/ShaderModule.h"

#include "Core/Assert.h"

ShaderModule::ShaderModule(const VulkanContext& context, const std::vector<uint8_t>& code)
   : device(context.device)
{
   ASSERT(code.size() > 0);

   vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
      .setCodeSize(code.size())
      .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

   shaderModule = device.createShaderModule(createInfo);
}

ShaderModule::ShaderModule(ShaderModule&& other)
{
   move(std::move(other));
}

ShaderModule::~ShaderModule()
{
   release();
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other)
{
   move(std::move(other));
   release();

   return *this;
}

void ShaderModule::move(ShaderModule&& other)
{
   ASSERT(!device);
   device = other.device;
   other.device = nullptr;

   ASSERT(!shaderModule);
   shaderModule = other.shaderModule;
   other.shaderModule = nullptr;
}

void ShaderModule::release()
{
   if (device)
   {
      ASSERT(shaderModule);
      device.destroyShaderModule(shaderModule);
      shaderModule = nullptr;
   }
}
