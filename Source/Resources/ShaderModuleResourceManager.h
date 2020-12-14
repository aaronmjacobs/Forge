#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/ShaderModule.h"

#include <filesystem>

struct VulkanContext;

using ShaderModuleHandle = ResourceManagerBase<ShaderModule>::Handle;

class ShaderModuleResourceManager : public ResourceManagerBase<ShaderModule>
{
public:
   ShaderModuleHandle load(const std::filesystem::path& path, const VulkanContext& context);
};
