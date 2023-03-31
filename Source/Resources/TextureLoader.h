#pragma once

#include "Core/Hash.h"

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
   bool generateMipMaps = true;

   bool operator==(const TextureLoadOptions& other) const = default;
};

struct TextureKey
{
   std::string canonicalPath;
   TextureLoadOptions options;

   std::size_t hash() const;
   bool operator==(const TextureKey& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(TextureKey);

class TextureLoader : public ResourceLoader<TextureKey, Texture>
{
public:
   TextureLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   TextureHandle load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {});

   TextureHandle createDefault(DefaultTextureType type);

private:
   const std::string& getDefaultPath(DefaultTextureType type) const;
   const std::string& getDefaultName(DefaultTextureType type) const;
};
