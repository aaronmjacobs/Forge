#pragma once

#include "Core/Delegate.h"
#include "Core/Hash.h"
#include "Core/Task.h"

#include "Resources/ResourceLoader.h"

#include "Graphics/Texture.h"

#include <memory>
#include <string>
#include <unordered_map>

class Image;

enum class DefaultTextureType
{
   None,
   Black,
   White,
   Normal,
   AoRoughnessMetalness,
   Cube,
   Volume
};

struct TextureLoadOptions
{
   bool sRGB = true;
   bool generateMipMaps = true;
   DefaultTextureType fallbackDefaultTextureType = DefaultTextureType::Black;

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
   ~TextureLoader();

   void update();

   TextureHandle load(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {});

   Texture* getDefault(DefaultTextureType type);
   const Texture* getDefault(DefaultTextureType type) const;

   using ReplaceDelegate = MulticastDelegate<void, TextureHandle>;
   DelegateHandle registerReplaceDelegate(TextureHandle textureHandle, ReplaceDelegate::FuncType function);
   void unregisterReplaceDelegate(TextureHandle textureHandle, DelegateHandle& delegateHandle);

private:
   struct LoadResult
   {
      std::unique_ptr<Image> image;
      std::string canonicalPath;
      TextureLoadOptions loadOptions;
      TextureHandle handle;
   };

   void onImageLoaded(LoadResult loadResult);
   std::unique_ptr<Texture> createDefault(DefaultTextureType type) const;

   std::unique_ptr<Texture> defaultBlack;
   std::unique_ptr<Texture> defaultWhite;
   std::unique_ptr<Texture> defaultNormal;
   std::unique_ptr<Texture> defaultAoRoughnessMetalness;
   std::unique_ptr<Texture> defaultCube;
   std::unique_ptr<Texture> defaultVolume;

   std::vector<Task<LoadResult>> loadTasks;
   std::unordered_map<Handle, ReplaceDelegate> replaceDelegates;
};
