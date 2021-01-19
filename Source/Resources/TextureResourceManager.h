#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Texture.h"

#include <filesystem>
#include <string>

class TextureResourceManager : public ResourceManagerBase<Texture, std::string>
{
public:
   TextureResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   TextureHandle load(const std::filesystem::path& path, const TextureProperties& properties = getDefaultProperties(), const TextureInitialLayout& initialLayout = getDefaultInitialLayout());

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();
};
