#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Texture.h"

#include <filesystem>

struct VulkanContext;

using TextureHandle = ResourceManagerBase<Texture>::Handle;

class TextureResourceManager : public ResourceManagerBase<Texture>
{
public:
   TextureHandle load(const std::filesystem::path& path, const VulkanContext& context, const TextureProperties& properties = getDefaultProperties(), const TextureInitialLayout& initialLayout = getDefaultInitialLayout());

   static TextureProperties getDefaultProperties();
   static TextureInitialLayout getDefaultInitialLayout();
};
