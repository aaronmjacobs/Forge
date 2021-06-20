#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Texture.h"

#include <filesystem>
#include <string>

struct TextureLoadOptions
{
   bool sRGB = true;
};

class TextureResourceManager : public ResourceManagerBase<Texture, std::string>
{
public:
   TextureResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   TextureHandle load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {}, const TextureProperties & properties = getDefaultProperties(), const TextureInitialLayout & initialLayout = getDefaultInitialLayout());

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();
};
