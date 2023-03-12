#pragma once

#include "Core/Hash.h"

#include "Graphics/DynamicDescriptorPool.h"
#include "Graphics/Material.h"

#include "Resources/ResourceLoader.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct TextureMaterialParameter
{
   std::string name;
   TextureHandle value;
   bool interpretAlphaAsMask = false;

   bool operator==(const TextureMaterialParameter& other) const = default;
   std::size_t hash() const;
};

USE_MEMBER_HASH_FUNCTION(TextureMaterialParameter);

struct VectorMaterialParameter
{
   std::string name;
   glm::vec4 value;

   bool operator==(const VectorMaterialParameter& other) const = default;
   std::size_t hash() const;
};

USE_MEMBER_HASH_FUNCTION(VectorMaterialParameter);

struct ScalarMaterialParameter
{
   std::string name;
   float value = 0.0f;

   bool operator==(const ScalarMaterialParameter& other) const = default;
   std::size_t hash() const;
};

USE_MEMBER_HASH_FUNCTION(ScalarMaterialParameter);

struct MaterialParameters
{
   std::vector<TextureMaterialParameter> textureParameters;
   std::vector<VectorMaterialParameter> vectorParameters;
   std::vector<ScalarMaterialParameter> scalarParameters;
   bool twoSided = false;

   bool operator==(const MaterialParameters& other) const = default;
   std::size_t hash() const;
};

USE_MEMBER_HASH_FUNCTION(MaterialParameters);

class MaterialLoader : public ResourceLoader<Material, MaterialParameters>
{
public:
   MaterialLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);
   ~MaterialLoader();

   void updateMaterials();

   MaterialHandle load(const MaterialParameters& parameters);

   void requestSetOfUpdates(Handle handle);

   DynamicDescriptorPool& getDynamicDescriptorPool()
   {
      return dynamicDescriptorPool;
   }

   vk::Sampler getSampler() const
   {
      return sampler;
   }

private:
   std::unique_ptr<Material> createMaterial(const MaterialParameters& parameters);

   std::unordered_map<Handle, int> materialsToUpdate;

   DynamicDescriptorPool dynamicDescriptorPool;
   vk::Sampler sampler;
};
