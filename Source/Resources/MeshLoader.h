#pragma once

#include "Core/Delegate.h"
#include "Core/Hash.h"
#include "Core/Task.h"

#include "Resources/MaterialLoader.h"
#include "Resources/ResourceLoader.h"
#include "Resources/TextureLoader.h"

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

   void update();

   using LoadDelegate = Delegate<void, MeshHandle>;
   MeshHandle load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {}, LoadDelegate&& loadDelegate = {});

   struct TextureInfo
   {
      std::filesystem::path path;
      TextureLoadOptions loadOptions;
      bool interpretAlphaAsMask = false;
   };

   struct MaterialInfo
   {
      TextureInfo albedo;
      TextureInfo normal;
      TextureInfo aoRoughnessMetalness;

      std::vector<VectorMaterialParameter> vectorParameters;
      std::vector<ScalarMaterialParameter> scalarParameters;

      bool twoSided = false;
   };

   struct SectionInfo
   {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;
      bool hasValidTexCoords = false;
      Bounds bounds;
      MaterialInfo materialInfo;
   };

private:
   struct LoadResult
   {
      std::vector<SectionInfo> sectionInfo;
      std::string canonicalPath;
      LoadDelegate loadDelegate;
      MeshHandle handle;
   };

   void onMeshLoaded(LoadResult result);

   std::unique_ptr<Mesh> defaultMesh;

   std::vector<Task<LoadResult>> loadTasks;
};
