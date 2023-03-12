#pragma once

#include "Resources/ResourceLoader.h"

#include "Graphics/ShaderModule.h"

#include <filesystem>
#include <string>

class ShaderModuleLoader : public ResourceLoader<ShaderModule, std::string>
{
public:
   ShaderModuleLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   ShaderModuleHandle load(const std::filesystem::path& path);
};
