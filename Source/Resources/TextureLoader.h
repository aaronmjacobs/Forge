#pragma once

#include "Resources/ResourceLoader.h"

#include "Graphics/Texture.h"

#include <string>

enum class DefaultTextureType
{
   None,
   Black,
   White,
   NormalMap,
   AoRoughnessMetalnessMap
};

struct TextureLoadOptions
{
   bool sRGB = true;
   DefaultTextureType fallbackDefaultTextureType = DefaultTextureType::None;
};

class TextureLoader : public ResourceLoader<Texture, std::string>
{
public:
   TextureLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   TextureHandle load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {}, const TextureProperties& properties = getDefaultProperties(), const TextureInitialLayout& initialLayout = getDefaultInitialLayout());

   TextureHandle createDefault(DefaultTextureType type);

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();

private:
   const std::string& getDefaultPath(DefaultTextureType type) const;
   const std::string& getDefaultName(DefaultTextureType type) const;
};
