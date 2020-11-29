#pragma once

#include "Resources/ResourceManagerBase.h"

#include "Graphics/Mesh.h"

#include <filesystem>

struct VulkanContext;

using MeshHandle = ResourceManagerBase<Mesh>::Handle;

class MeshResourceManager : public ResourceManagerBase<Mesh>
{
public:
   MeshHandle load(const std::filesystem::path& path, const VulkanContext& context);
};
