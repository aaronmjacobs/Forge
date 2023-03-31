#pragma once

#include "Core/Hash.h"

#include "Resources/ResourceLoader.h"

#include "Graphics/Mesh.h"

#include <filesystem>
#include <string>

enum class MeshAxis
{
   PositiveX,
   PositiveY,
   PositiveZ,
   NegativeX,
   NegativeY,
   NegativeZ,
};

struct MeshLoadOptions
{
   MeshAxis forwardAxis = MeshAxis::NegativeZ;
   MeshAxis upAxis = MeshAxis::PositiveY;
   float scale = 1.0f;
   bool interpretTextureAlphaAsMask = false;

   bool operator==(const MeshLoadOptions& other) const = default;
};

struct MeshKey
{
   std::string canonicalPath;
   MeshLoadOptions options;

   std::size_t hash() const;
   bool operator==(const MeshKey& other) const = default;
};

USE_MEMBER_HASH_FUNCTION(MeshKey);

class MeshLoader : public ResourceLoader<MeshKey, Mesh>
{
public:
   MeshLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   MeshHandle load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {});
};
