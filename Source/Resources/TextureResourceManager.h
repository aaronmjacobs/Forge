#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Texture.h"

#include <filesystem>
#include <string>

enum class DefaultTextureType
{
   None,
   Black,
   White,
   NormalMap
};

struct TextureLoadOptions
{
   bool sRGB = true;
   DefaultTextureType fallbackDefaultTextureType = DefaultTextureType::None;
};

class TextureResourceManager : public ResourceManagerBase<Texture, std::string>
{
public:
   TextureResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   TextureHandle load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {}, const TextureProperties & properties = getDefaultProperties(), const TextureInitialLayout & initialLayout = getDefaultInitialLayout());

   TextureHandle getDefault(DefaultTextureType type) const;

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();

protected:
   void onAllResourcesUnloaded() override;
   void createDefaultTextures();

   TextureHandle defaultBlackTextureHandle;
   TextureHandle defaultWhiteTextureHandle;
   TextureHandle defaultNormalMapTextureHandle;
};
