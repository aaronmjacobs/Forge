#pragma once

#include "Resources/ResourceLoader.h"

#include "Graphics/ShaderModule.h"

#include <filesystem>
#include <string>

class ShaderModuleLoader : public ResourceLoader<std::string, ShaderModule>
{
public:
   ShaderModuleLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   ShaderModuleHandle load(const std::filesystem::path& path);
};
