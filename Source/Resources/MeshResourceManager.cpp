#include "Resources/MeshResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <PlatformUtils/IOUtils.h>

namespace
{
   MeshSectionSourceData processAssimpMesh(const aiMesh& assimpMesh)
   {
      MeshSectionSourceData sectionSourceData;

      static_assert(sizeof(unsigned int) == sizeof(uint32_t), "Index data types don't match");
      sectionSourceData.indices = std::vector<uint32_t>(assimpMesh.mNumFaces * 3);
      for (unsigned int i = 0; i < assimpMesh.mNumFaces; ++i)
      {
         const aiFace& face = assimpMesh.mFaces[i];
         ASSERT(face.mNumIndices == 3);

         std::memcpy(&sectionSourceData.indices[i * 3], face.mIndices, 3 * sizeof(uint32_t));
      }

      if (assimpMesh.mNumVertices > 0 && assimpMesh.mTextureCoords[0] && assimpMesh.mNumUVComponents[0] == 2)
      {
         sectionSourceData.vertices.reserve(assimpMesh.mNumVertices);

         for (unsigned int i = 0; i < assimpMesh.mNumVertices; ++i)
         {
            glm::vec3 position(assimpMesh.mVertices[i].x, assimpMesh.mVertices[i].y, assimpMesh.mVertices[i].z);
            glm::vec3 color(1.0f);
            glm::vec2 texCoord(assimpMesh.mTextureCoords[0][i].x, assimpMesh.mTextureCoords[0][i].y);

            sectionSourceData.vertices.push_back(Vertex(position, color, texCoord));
         }
      }

      return sectionSourceData;
   }

   void processAssimpNode(std::vector<MeshSectionSourceData>& sourceData, const aiScene& assimpScene, const aiNode& assimpNode)
   {
      for (unsigned int i = 0; i < assimpNode.mNumMeshes; ++i)
      {
         const aiMesh& assimpMesh = *assimpScene.mMeshes[assimpNode.mMeshes[i]];
         sourceData.push_back(processAssimpMesh(assimpMesh));
      }

      for (unsigned int i = 0; i < assimpNode.mNumChildren; ++i)
      {
         processAssimpNode(sourceData, assimpScene, *assimpNode.mChildren[i]);
      }
   }

   std::vector<MeshSectionSourceData> loadMesh(const std::filesystem::path& path)
   {
      std::vector<MeshSectionSourceData> sourceData;

      unsigned int flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_FlipUVs;

      Assimp::Importer importer;
      const aiScene* assimpScene = importer.ReadFile(path.string().c_str(), flags);
      if (assimpScene && assimpScene->mRootNode && !(assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
      {
         processAssimpNode(sourceData, *assimpScene, *assimpScene->mRootNode);
      }

      return sourceData;
   }
}

MeshHandle MeshResourceManager::load(const std::filesystem::path& path, const GraphicsContext& context)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceHelpers::makeCanonical(path))
   {
      std::string canonicalPathString = canonicalPath->string();
      if (std::optional<Handle> cachedHandle = getCachedHandle(canonicalPathString))
      {
         return *cachedHandle;
      }

      std::vector<MeshSectionSourceData> sourceData = loadMesh(*canonicalPath);
      if (!sourceData.empty())
      {
         MeshHandle handle = emplaceResource(context, sourceData);
         cacheHandle(handle, canonicalPathString);

         return handle;
      }
   }

   return MeshHandle();
}
