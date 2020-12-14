#pragma once

#include "Graphics/Context.h"

#include <cstdint>
#include <vector>

class ShaderModule
{
public:
   ShaderModule(const VulkanContext& context, const std::vector<uint8_t>& code);

   ShaderModule(const ShaderModule& other) = delete;
   ShaderModule(ShaderModule&& other);

   ~ShaderModule();

   ShaderModule& operator=(const ShaderModule& other) = delete;
   ShaderModule& operator=(ShaderModule&& other);

private:
   void move(ShaderModule&& other);
   void release();

public:
   vk::ShaderModule getShaderModule() const
   {
      return shaderModule;
   }

private:
   vk::Device device;

   vk::ShaderModule shaderModule;
};
