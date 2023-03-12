#pragma once

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
};

class MeshLoader : public ResourceLoader<Mesh, std::string>
{
public:
   MeshLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   MeshHandle load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {});
};
