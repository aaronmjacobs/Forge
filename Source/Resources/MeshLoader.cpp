#include "Resources/MeshLoader.h"

#include "Core/Enum.h"

#include "Graphics/DebugUtils.h"

#include "Math/MathUtils.h"

#include "Renderer/PhysicallyBasedMaterial.h"

#include "Resources/ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>

#include <PlatformUtils/IOUtils.h>

#include <span>
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

   StrongTextureHandle loadMaterialTexture(const aiMaterial& assimpMaterial, std::span<const aiTextureType> textureTypes, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      std::filesystem::path texturePath;
      aiTextureType textureType = textureTypes.empty() ? aiTextureType_NONE : textureTypes[0];

      for (aiTextureType type : textureTypes)
      {
         if (assimpMaterial.GetTextureCount(type) > 0)
         {
            aiString textureName;
            if (assimpMaterial.GetTexture(type, 0, &textureName) == aiReturn_SUCCESS)
            {
               texturePath = directory / textureName.C_Str();
               textureType = type;
               break;
            }
         }
      }

      TextureLoadOptions loadOptions;
      loadOptions.sRGB = textureType == aiTextureType_BASE_COLOR || textureType == aiTextureType_DIFFUSE;

      switch (textureType)
      {
      case aiTextureType_BASE_COLOR:
      case aiTextureType_DIFFUSE:
         loadOptions.fallbackDefaultTextureType = DefaultTextureType::White;
         break;
      case aiTextureType_NORMALS:
         loadOptions.fallbackDefaultTextureType = DefaultTextureType::Normal;
         break;
      case aiTextureType_AMBIENT_OCCLUSION:
      case aiTextureType_DIFFUSE_ROUGHNESS:
      case aiTextureType_METALNESS:
      case aiTextureType_UNKNOWN:
         loadOptions.fallbackDefaultTextureType = DefaultTextureType::AoRoughnessMetalness;
         break;
      default:
         loadOptions.fallbackDefaultTextureType = DefaultTextureType::Black;
         break;
      }

      return resourceManager.loadTexture(texturePath, loadOptions);
   }

   StrongMaterialHandle processAssimpMaterial(const aiMaterial& assimpMaterial, bool interpretTextureAlphaAsMask, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      static const std::array<aiTextureType, 2> kAlbedoTextureTypes = { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE };
      static const std::array<aiTextureType, 1> kNormalTextureTypes = { aiTextureType_NORMALS };
      static const std::array<aiTextureType, 4> kAoRMTextureTypes = { aiTextureType_AMBIENT_OCCLUSION, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_METALNESS, aiTextureType_UNKNOWN };
      
      StrongTextureHandle albedoTextureHandle = loadMaterialTexture(assimpMaterial, kAlbedoTextureTypes, directory, resourceManager);
      StrongTextureHandle normalTextureHandle = loadMaterialTexture(assimpMaterial, kNormalTextureTypes, directory, resourceManager);
      StrongTextureHandle aoRoughnessMetalnessTextureHandle = loadMaterialTexture(assimpMaterial, kAoRMTextureTypes, directory, resourceManager);

      TextureMaterialParameter albedoParameter;
      albedoParameter.name = PhysicallyBasedMaterial::kAlbedoTextureParameterName;
      albedoParameter.value = albedoTextureHandle;
      albedoParameter.interpretAlphaAsMask = interpretTextureAlphaAsMask;

      TextureMaterialParameter normalParameter;
      normalParameter.name = PhysicallyBasedMaterial::kNormalTextureParameterName;
      normalParameter.value = normalTextureHandle;
      
      TextureMaterialParameter aoRoughnessMetalnessParameter;
      aoRoughnessMetalnessParameter.name = PhysicallyBasedMaterial::kAoRoughnessMetalnessTextureParameterName;
      aoRoughnessMetalnessParameter.value = aoRoughnessMetalnessTextureHandle;

      MaterialParameters materialParameters;
      materialParameters.textureParameters.push_back(albedoParameter);
      materialParameters.textureParameters.push_back(normalParameter);
      materialParameters.textureParameters.push_back(aoRoughnessMetalnessParameter);

      int twoSided = 0;
      if (assimpMaterial.Get(AI_MATKEY_TWOSIDED, twoSided) == aiReturn_SUCCESS)
      {
         materialParameters.twoSided = twoSided != 0;
      }

      aiColor4D albedoColor(0.0f);
      if (assimpMaterial.Get(AI_MATKEY_BASE_COLOR, albedoColor) == aiReturn_SUCCESS || assimpMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, albedoColor) == aiReturn_SUCCESS)
      {
         materialParameters.vectorParameters.push_back(VectorMaterialParameter{ PhysicallyBasedMaterial::kAlbedoTextureParameterName, glm::vec4(albedoColor.r, albedoColor.g, albedoColor.b, albedoColor.a) });
      }

      float emissiveIntensity = 1.0f;
      assimpMaterial.Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity);

      aiColor4D emissiveColor(0.0f);
      if (assimpMaterial.Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == aiReturn_SUCCESS)
      {
         materialParameters.vectorParameters.push_back(VectorMaterialParameter{ PhysicallyBasedMaterial::kEmissiveVectorParameterName, glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, emissiveColor.a) * emissiveIntensity });
      }

      float roughness = 0.0f;
      if (assimpMaterial.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
      {
         materialParameters.scalarParameters.push_back(ScalarMaterialParameter{ PhysicallyBasedMaterial::kRoughnessScalarParameterName, roughness });
      }

      float metalness = 0.0f;
      if (assimpMaterial.Get(AI_MATKEY_METALLIC_FACTOR, metalness) == aiReturn_SUCCESS)
      {
         materialParameters.scalarParameters.push_back(ScalarMaterialParameter{ PhysicallyBasedMaterial::kMetalnessScalarParameterName, metalness });
      }

      return resourceManager.loadMaterial(materialParameters);
   }

   MeshSectionSourceData processAssimpMesh(const aiScene& assimpScene, const aiMesh& assimpMesh, const glm::mat3& swizzle, float scale, bool interpretTextureAlphaAsMask, const std::filesystem::path& directory, ResourceManager& resourceManager)
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
         sectionSourceData.materialHandle = processAssimpMaterial(*assimpScene.mMaterials[assimpMesh.mMaterialIndex], interpretTextureAlphaAsMask, directory, resourceManager);
      }

      return sectionSourceData;
   }

   void processAssimpNode(std::vector<MeshSectionSourceData>& sourceData, const aiScene& assimpScene, const aiNode& assimpNode, const glm::mat3& swizzle, float scale, bool interpretTextureAlphaAsMask, const std::filesystem::path& directory, ResourceManager& resourceManager)
   {
      for (unsigned int i = 0; i < assimpNode.mNumMeshes; ++i)
      {
         const aiMesh& assimpMesh = *assimpScene.mMeshes[assimpNode.mMeshes[i]];
         sourceData.push_back(processAssimpMesh(assimpScene, assimpMesh, swizzle, scale, interpretTextureAlphaAsMask, directory, resourceManager));
      }

      for (unsigned int i = 0; i < assimpNode.mNumChildren; ++i)
      {
         processAssimpNode(sourceData, assimpScene, *assimpNode.mChildren[i], swizzle, scale, interpretTextureAlphaAsMask, directory, resourceManager);
      }
   }

   std::vector<MeshSectionSourceData> loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions, ResourceManager& resourceManager)
   {
      std::vector<MeshSectionSourceData> sourceData;

      unsigned int flags = aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices | aiProcess_FlipUVs;

      Assimp::Importer importer;
      const aiScene* assimpScene = importer.ReadFile(path.string().c_str(), flags);
      if (assimpScene && assimpScene->mRootNode && !(assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
      {
         std::filesystem::path directory = path.parent_path();
         processAssimpNode(sourceData, *assimpScene, *assimpScene->mRootNode, getSwizzleMatrix(loadOptions), loadOptions.scale, loadOptions.interpretTextureAlphaAsMask, directory, resourceManager);
      }

      return sourceData;
   }
}

std::size_t MeshKey::hash() const
{
   std::size_t hash = 0;

   Hash::combine(hash, canonicalPath);

   Hash::combine(hash, options.forwardAxis);
   Hash::combine(hash, options.upAxis);
   Hash::combine(hash, options.scale);
   Hash::combine(hash, options.interpretTextureAlphaAsMask);

   return hash;
}

MeshLoader::MeshLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceLoader(graphicsContext, owningResourceManager)
{
}

MeshHandle MeshLoader::load(const std::filesystem::path& path, const MeshLoadOptions& loadOptions)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceLoadHelpers::makeCanonical(path))
   {
      MeshKey key;
      key.canonicalPath = canonicalPath->string();
      key.options = loadOptions;

      if (Handle cachedHandle = container.findHandle(key))
      {
         return cachedHandle;
      }

      std::vector<MeshSectionSourceData> sourceData = loadMesh(*canonicalPath, loadOptions, resourceManager);
      if (!sourceData.empty())
      {
         MeshHandle handle = container.emplace(key, context, sourceData);
         NAME_POINTER(context.getDevice(), get(handle), ResourceLoadHelpers::getName(*canonicalPath));

         return handle;
      }
   }

   return MeshHandle{};
}
