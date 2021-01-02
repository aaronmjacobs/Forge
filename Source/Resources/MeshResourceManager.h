#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Mesh.h"

#include <filesystem>

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
};

using MeshHandle = ResourceManagerBase<Mesh>::Handle;

class MeshResourceManager : public ResourceManagerBase<Mesh>
{
public:
   MeshResourceManager(const GraphicsContext& graphicsContext);

   MeshHandle load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {});
};
