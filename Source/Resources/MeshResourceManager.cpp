#include "Resources/MeshResourceManager.h"

#include "Core/Enum.h"
#include "Core/Math/MathUtils.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>

#include <PlatformUtils/IOUtils.h>

namespace
{
   glm::vec3 getMeshAxisVector(MeshAxis meshAxis)
   {
      switch (meshAxis)
      {
      case MeshAxis::PositiveX:
         return MathUtils::kRightVector;
      case MeshAxis::PositiveY:
         return MathUtils::kForwardVector;
      case MeshAxis::PositiveZ:
         return MathUtils::kUpVector;
      case MeshAxis::NegativeX:
         return -MathUtils::kRightVector;
      case MeshAxis::NegativeY:
         return -MathUtils::kForwardVector;
      case MeshAxis::NegativeZ:
         return -MathUtils::kUpVector;
      default:
         ASSERT(false);
         return glm::vec3(0.0f);
      }
   }

   MeshAxis getMeshAxis(const glm::vec3& vector)
   {
      if (vector == MathUtils::kForwardVector)
      {
         return MeshAxis::PositiveY;
      }
      else if (vector == -MathUtils::kForwardVector)
      {
         return MeshAxis::NegativeY;
      }
      else if (vector == MathUtils::kUpVector)
      {
         return MeshAxis::PositiveZ;
      }
      else if (vector == -MathUtils::kUpVector)
      {
         return MeshAxis::NegativeZ;
      }
      else if (vector == MathUtils::kRightVector)
      {
         return MeshAxis::PositiveX;
      }
      else if (vector == -MathUtils::kRightVector)
      {
         return MeshAxis::NegativeX;
      }

      ASSERT(false);
      return MeshAxis::PositiveX;
   }

   int getSwizzleIndex(MeshAxis meshAxis)
   {
      return Enum::cast(meshAxis) % 3;
   }

   float getSwizzleSign(MeshAxis meshAxis)
   {
      return Enum::cast(meshAxis) > 2 ? -1.0f : 1.0f;
   }

   glm::mat3 getSwizzleMatrix(const MeshLoadOptions& loadOptions)
   {
      glm::vec3 meshForward = getMeshAxisVector(loadOptions.forwardAxis);
      glm::vec3 meshUp = getMeshAxisVector(loadOptions.upAxis);
      glm::vec3 meshRight = glm::cross(meshForward, meshUp);

      MeshAxis rightAxis = getMeshAxis(meshRight);

      int forwardIndex = getSwizzleIndex(loadOptions.forwardAxis);
      int upIndex = getSwizzleIndex(loadOptions.upAxis);
      int rightIndex = getSwizzleIndex(rightAxis);
      ASSERT(forwardIndex != upIndex && forwardIndex != rightIndex && upIndex != rightIndex);

      glm::mat3 swizzle(1.0f);
      swizzle[forwardIndex] = MathUtils::kForwardVector * getSwizzleSign(loadOptions.forwardAxis);
      swizzle[upIndex] = MathUtils::kUpVector * getSwizzleSign(loadOptions.upAxis);
      swizzle[rightIndex] = MathUtils::kRightVector * getSwizzleSign(rightAxis);

      return swizzle;
   }

   MeshSectionSourceData processAssimpMesh(const aiMesh& assimpMesh, const glm::mat3& swizzle)
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

      if (assimpMesh.mNumVertices > 0)
      {
         sectionSourceData.vertices.reserve(assimpMesh.mNumVertices);
         bool hasTextureCoordinates = assimpMesh.mTextureCoords[0] && assimpMesh.mNumUVComponents[0] == 2;

         for (unsigned int i = 0; i < assimpMesh.mNumVertices; ++i)
         {
            glm::vec3 position = swizzle * glm::vec3(assimpMesh.mVertices[i].x, assimpMesh.mVertices[i].y, assimpMesh.mVertices[i].z);
            glm::vec3 color(1.0f);
            glm::vec2 texCoord = hasTextureCoordinates ? glm::vec2(assimpMesh.mTextureCoords[0][i].x, assimpMesh.mTextureCoords[0][i].y) : glm::vec2(0.0f, 0.0f);

            sectionSourceData.vertices.push_back(Vertex(position, color, texCoord));
         }
      }

      return sectionSourceData;
   }

   void processAssimpNode(std::vector<MeshSectionSourceData>& sourceData, const aiScene& assimpScene, const aiNode& assimpNode, const glm::mat3& swizzle)
   {
      for (unsigned int i = 0; i < assimpNode.mNumMeshes; ++i)
      {
         const aiMesh& assimpMesh = *assimpScene.mMeshes[assimpNode.mMeshes[i]];
         sourceData.push_back(processAssimpMesh(assimpMesh, swizzle));
      }

      for (unsigned int i = 0; i < assimpNode.mNumChildren; ++i)
      {
         processAssimpNode(sourceData, assimpScene, *assimpNode.mChildren[i], swizzle);
      }
   }

   std::vector<MeshSectionSourceData> loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions)
   {
      std::vector<MeshSectionSourceData> sourceData;

      unsigned int flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_FlipUVs;

      Assimp::Importer importer;
      const aiScene* assimpScene = importer.ReadFile(path.string().c_str(), flags);
      if (assimpScene && assimpScene->mRootNode && !(assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
      {
         processAssimpNode(sourceData, *assimpScene, *assimpScene->mRootNode, getSwizzleMatrix(loadOptions));
      }

      return sourceData;
   }
}

MeshResourceManager::MeshResourceManager(const GraphicsContext& graphicsContext)
   : ResourceManagerBase(graphicsContext)
{
}

MeshHandle MeshResourceManager::load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceHelpers::makeCanonical(path))
   {
      std::string canonicalPathString = canonicalPath->string();
      if (std::optional<Handle> cachedHandle = getCachedHandle(canonicalPathString))
      {
         return *cachedHandle;
      }

      std::vector<MeshSectionSourceData> sourceData = loadMesh(*canonicalPath, loadOptions);
      if (!sourceData.empty())
      {
         MeshHandle handle = emplaceResource(context, sourceData);
         cacheHandle(handle, canonicalPathString);

         return handle;
      }
   }

   return MeshHandle();
}
