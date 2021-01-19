#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/ShaderModule.h"

#include <filesystem>
#include <string>

class ShaderModuleResourceManager : public ResourceManagerBase<ShaderModule, std::string>
{
public:
   ShaderModuleResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   ShaderModuleHandle load(const std::filesystem::path& path);
};
