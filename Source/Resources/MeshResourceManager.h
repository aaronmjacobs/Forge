#pragma once

#include "Resources/ResourceManagerBase.h"

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

class MeshResourceManager : public ResourceManagerBase<Mesh, std::string>
{
public:
   MeshResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);

   MeshHandle load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {});
};
