#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Texture.h"

#include <filesystem>

using TextureHandle = ResourceManagerBase<Texture>::Handle;

class TextureResourceManager : public ResourceManagerBase<Texture>
{
public:
   TextureResourceManager(const GraphicsContext& graphicsContext);

   TextureHandle load(const std::filesystem::path& path, const TextureProperties& properties = getDefaultProperties(), const TextureInitialLayout& initialLayout = getDefaultInitialLayout());

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();
};
