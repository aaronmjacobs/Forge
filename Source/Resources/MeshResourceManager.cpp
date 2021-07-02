#include "Resources/MeshResourceManager.h"

#include "Core/Enum.h"

#include "Math/MathUtils.h"

#include "Renderer/PhongMaterial.h"

#include "Resources/ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>

#include <PlatformUtils/IOUtils.h>

#include <limits>

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

   TextureHandle loadMaterialTexture(const aiMaterial& assimpMaterial, aiTextureType textureType, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      std::filesystem::path texturePath;

      if (assimpMaterial.GetTextureCount(textureType) > 0)
      {
         aiString textureName;
         if (assimpMaterial.GetTexture(textureType, 0, &textureName) == aiReturn_SUCCESS)
         {
            texturePath = directory / textureName.C_Str();
         }
      }

      TextureLoadOptions loadOptions;
      loadOptions.sRGB = textureType != aiTextureType_NORMALS;
      loadOptions.fallbackDefaultTextureType = textureType == aiTextureType_NORMALS ? DefaultTextureType::NormalMap : DefaultTextureType::White;

      return resourceManager.loadTexture(texturePath, loadOptions);
   }

   MaterialHandle processAssimpMaterial(const aiMaterial& assimpMaterial, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      TextureHandle diffuseTextureHandle = loadMaterialTexture(assimpMaterial, aiTextureType_DIFFUSE, directory, resourceManager);
      TextureHandle normalTextureHandle = loadMaterialTexture(assimpMaterial, aiTextureType_NORMALS, directory, resourceManager);

      TextureMaterialParameter diffuseParameter;
      diffuseParameter.name = PhongMaterial::kDiffuseTextureParameterName;
      diffuseParameter.value = diffuseTextureHandle;

      TextureMaterialParameter normalParameter;
      normalParameter.name = PhongMaterial::kNormalTextureParameterName;
      normalParameter.value = normalTextureHandle;

      MaterialParameters materialParameters;
      materialParameters.textureParameters.push_back(diffuseParameter);
      materialParameters.textureParameters.push_back(normalParameter);

      return resourceManager.loadMaterial(materialParameters);
   }

   MeshSectionSourceData processAssimpMesh(const aiScene& assimpScene, const aiMesh& assimpMesh, const glm::mat3& swizzle, float scale, const std::filesystem::path& directory, ResourceManager& resourceManager)
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
         sectionSourceData.vertices.resize(assimpMesh.mNumVertices);
         bool hasTextureCoordinates = assimpMesh.mTextureCoords[0] && assimpMesh.mNumUVComponents[0] == 2;
         sectionSourceData.hasValidTexCoords = hasTextureCoordinates;

         glm::vec3 minPosition(std::numeric_limits<float>::max());
         glm::vec3 maxPosition(std::numeric_limits<float>::lowest());

         for (unsigned int i = 0; i < assimpMesh.mNumVertices; ++i)
         {
            Vertex& vertex = sectionSourceData.vertices[i];

            vertex.position = swizzle * glm::vec3(assimpMesh.mVertices[i].x, assimpMesh.mVertices[i].y, assimpMesh.mVertices[i].z) * scale;

            if (assimpMesh.mNormals)
            {
               vertex.normal = swizzle * glm::vec3(assimpMesh.mNormals[i].x, assimpMesh.mNormals[i].y, assimpMesh.mNormals[i].z);
            }

            if (assimpMesh.mTangents)
            {
               vertex.tangent = swizzle * glm::vec3(assimpMesh.mTangents[i].x, assimpMesh.mTangents[i].y, assimpMesh.mTangents[i].z);
            }

            if (assimpMesh.mBitangents)
            {
               vertex.bitangent = swizzle * glm::vec3(assimpMesh.mBitangents[i].x, assimpMesh.mBitangents[i].y, assimpMesh.mBitangents[i].z);
            }

            if (assimpMesh.mColors[0])
            {
               vertex.color = glm::vec4(assimpMesh.mColors[0][i].r, assimpMesh.mColors[0][i].g, assimpMesh.mColors[0][i].b, assimpMesh.mColors[0][i].a);
            }
            else
            {
               vertex.color = glm::vec4(1.0f);
            }

            if (hasTextureCoordinates)
            {
               vertex.texCoord = glm::vec2(assimpMesh.mTextureCoords[0][i].x, assimpMesh.mTextureCoords[0][i].y);
            }

            minPosition = glm::min(minPosition, vertex.position);
            maxPosition = glm::max(maxPosition, vertex.position);
         }

         std::array<glm::vec3, 2> points = { minPosition, maxPosition };
         sectionSourceData.bounds = Bounds(points);
      }

      if (assimpMesh.mMaterialIndex < assimpScene.mNumMaterials && assimpScene.mMaterials[assimpMesh.mMaterialIndex])
      {
         sectionSourceData.materialHandle = processAssimpMaterial(*assimpScene.mMaterials[assimpMesh.mMaterialIndex], directory, resourceManager);
      }

      return sectionSourceData;
   }

   void processAssimpNode(std::vector<MeshSectionSourceData>& sourceData, const aiScene& assimpScene, const aiNode& assimpNode, const glm::mat3& swizzle, float scale, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      for (unsigned int i = 0; i < assimpNode.mNumMeshes; ++i)
      {
         const aiMesh& assimpMesh = *assimpScene.mMeshes[assimpNode.mMeshes[i]];
         sourceData.push_back(processAssimpMesh(assimpScene, assimpMesh, swizzle, scale, directory, resourceManager));
      }

      for (unsigned int i = 0; i < assimpNode.mNumChildren; ++i)
      {
         processAssimpNode(sourceData, assimpScene, *assimpNode.mChildren[i], swizzle, scale, directory, resourceManager);
      }
   }

   std::vector<MeshSectionSourceData> loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions, ResourceManager& resourceManager)
   {
      std::vector<MeshSectionSourceData> sourceData;

      unsigned int flags = aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs;

      Assimp::Importer importer;
      const aiScene* assimpScene = importer.ReadFile(path.string().c_str(), flags);
      if (assimpScene && assimpScene->mRootNode && !(assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
      {
         std::filesystem::path directory = path.parent_path();
         processAssimpNode(sourceData, *assimpScene, *assimpScene->mRootNode, getSwizzleMatrix(loadOptions), loadOptions.scale, directory, resourceManager);
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
