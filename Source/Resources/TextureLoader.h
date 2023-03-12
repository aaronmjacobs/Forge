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

   TextureHandle getDefault(DefaultTextureType type) const;
   std::unique_ptr<Texture> createDefault(DefaultTextureType type) const;

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();

protected:
   void onAllResourcesUnloaded() override;
   void createDefaultTextures();

   TextureHandle defaultBlackTextureHandle;
   TextureHandle defaultWhiteTextureHandle;
   TextureHandle defaultNormalMapTextureHandle;
   TextureHandle defaultAoRoughnessMetalnessMapTextureHandle;
};
