#pragma once

#include "Graphics/DynamicDescriptorPool.h"
#include "Graphics/Material.h"

#include "Resources/ResourceManagerBase.h"
#include "Resources/TextureResourceManager.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct TextureMaterialParameter
{
   std::string name;
   TextureHandle value;
   bool interpretAlphaAsMask = false;

   bool operator==(const TextureMaterialParameter& other) const;
};

struct VectorMaterialParameter
{
   std::string name;
   glm::vec4 value;

   bool operator==(const VectorMaterialParameter& other) const;
};

struct FloatMaterialParameter
{
   std::string name;
   float value = 0.0f;

   bool operator==(const FloatMaterialParameter& other) const;
};

struct MaterialParameters
{
   std::vector<TextureMaterialParameter> textureParameters;
   std::vector<VectorMaterialParameter> vectorParameters;
   std::vector<FloatMaterialParameter> scalarParameters;

   bool operator==(const MaterialParameters& other) const;
};

namespace std
{
   template<>
   struct hash<TextureMaterialParameter>
   {
      size_t operator()(const TextureMaterialParameter& textureMaterialParameters) const;
   };

   template<>
   struct hash<VectorMaterialParameter>
   {
      size_t operator()(const VectorMaterialParameter& vectorMaterialParameters) const;
   };

   template<>
   struct hash<FloatMaterialParameter>
   {
      size_t operator()(const FloatMaterialParameter& floatMaterialParameters) const;
   };

   template<>
   struct hash<MaterialParameters>
   {
      size_t operator()(const MaterialParameters& materialParameters) const;
   };
}

class MaterialResourceManager : public ResourceManagerBase<Material, MaterialParameters>
{
public:
   MaterialResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager);
   ~MaterialResourceManager();

   MaterialHandle load(const MaterialParameters& parameters);

private:
   std::unique_ptr<Material> createMaterial(const MaterialParameters& parameters);

   DynamicDescriptorPool dynamicDescriptorPool;
   vk::Sampler sampler;
};
