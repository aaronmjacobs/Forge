#include "Resources/MeshResourceManager.h"

#include "Core/Enum.h"

#include "Math/MathUtils.h"

#include "Resources/ResourceManager.h"

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
      swizzle[forwardIndex] = MathUtils::kForwardVector * getSwizzleSign(loadOptions.forwardAxis) * loadOptions.scale;
      swizzle[upIndex] = MathUtils::kUpVector * getSwizzleSign(loadOptions.upAxis) * loadOptions.scale;
      swizzle[rightIndex] = MathUtils::kRightVector * getSwizzleSign(rightAxis) * loadOptions.scale;

      return swizzle;
   }

   TextureHandle loadMaterialTexture(const aiMaterial& assimpMaterial, aiTextureType textureType, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      if (assimpMaterial.GetTextureCount(textureType) > 0)
      {
         aiString textureName;
         if (assimpMaterial.GetTexture(textureType, 0, &textureName) == aiReturn_SUCCESS)
         {
            std::filesystem::path texturePath = directory / textureName.C_Str();
            return resourceManager.loadTexture(texturePath);
         }
      }

      return TextureHandle();
   }

   MaterialHandle processAssimpMaterial(const aiMaterial& assimpMaterial, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      TextureHandle diffuseTextureHandle = loadMaterialTexture(assimpMaterial, aiTextureType_DIFFUSE, directory, resourceManager);
      TextureHandle specularTextureHandle = loadMaterialTexture(assimpMaterial, aiTextureType_SPECULAR, directory, resourceManager);
      TextureHandle normalTextureHandle = loadMaterialTexture(assimpMaterial, aiTextureType_NORMALS, directory, resourceManager);
      if (!normalTextureHandle)
      {
         normalTextureHandle = loadMaterialTexture(assimpMaterial, aiTextureType_HEIGHT, directory, resourceManager);
      }

      // TODO: Proper shading model
      TextureMaterialParameter textureParameter;
      textureParameter.name = "texture";
      textureParameter.value = diffuseTextureHandle;

      MaterialParameters materialParameters;
      materialParameters.textureParameters.push_back(textureParameter);

      return resourceManager.loadMaterial(materialParameters);
   }

   MeshSectionSourceData processAssimpMesh(const aiScene& assimpScene, const aiMesh& assimpMesh, const glm::mat3& swizzle, const std::filesystem::path& directory, ResourceManager& resourceManager)
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

      if (assimpMesh.mMaterialIndex < assimpScene.mNumMaterials && assimpScene.mMaterials[assimpMesh.mMaterialIndex])
      {
         sectionSourceData.materialHandle = processAssimpMaterial(*assimpScene.mMaterials[assimpMesh.mMaterialIndex], directory, resourceManager);
      }

      return sectionSourceData;
   }

   void processAssimpNode(std::vector<MeshSectionSourceData>& sourceData, const aiScene& assimpScene, const aiNode& assimpNode, const glm::mat3& swizzle, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      for (unsigned int i = 0; i < assimpNode.mNumMeshes; ++i)
      {
         const aiMesh& assimpMesh = *assimpScene.mMeshes[assimpNode.mMeshes[i]];
         sourceData.push_back(processAssimpMesh(assimpScene, assimpMesh, swizzle, directory, resourceManager));
      }

      for (unsigned int i = 0; i < assimpNode.mNumChildren; ++i)
      {
         processAssimpNode(sourceData, assimpScene, *assimpNode.mChildren[i], swizzle, directory, resourceManager);
      }
   }

   std::vector<MeshSectionSourceData> loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions, ResourceManager& resourceManager)
   {
      std::vector<MeshSectionSourceData> sourceData;

      unsigned int flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_FlipUVs;

      Assimp::Importer importer;
      const aiScene* assimpScene = importer.ReadFile(path.string().c_str(), flags);
      if (assimpScene && assimpScene->mRootNode && !(assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
      {
         std::filesystem::path directory = path.parent_path();
         processAssimpNode(sourceData, *assimpScene, *assimpScene->mRootNode, getSwizzleMatrix(loadOptions), directory, resourceManager);
      }

      return sourceData;
   }
}

MeshResourceManager::MeshResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceManagerBase(graphicsContext, owningResourceManager)
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

      std::vector<MeshSectionSourceData> sourceData = loadMesh(*canonicalPath, loadOptions, resourceManager);
      if (!sourceData.empty())
      {
         MeshHandle handle = emplaceResource(context, sourceData);
         cacheHandle(canonicalPathString, handle);

         return handle;
      }
   }

   return MeshHandle();
}
