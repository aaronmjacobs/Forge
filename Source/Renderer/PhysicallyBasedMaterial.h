#pragma once

#include "Graphics/Material.h"
#include "Graphics/UniformBuffer.h"

#include <glm/glm.hpp>

class Texture;

struct PhysicallyBasedMaterialUniformData
{
   alignas(16) glm::vec4 albedo = glm::vec4(1.0f);
   alignas(16) glm::vec4 emissive = glm::vec4(0.0f);
   alignas(4) float emissiveIntensity = 0.0f;
   alignas(4) float roughness = 0.5f;
   alignas(4) float metalness = 0.0f;
   alignas(4) float ambientOcclusion = 1.0f;
};

struct PhysicallyBasedMaterialParams
{
   const Texture* albedoTexture = nullptr;
   const Texture* normalTexture = nullptr;
   const Texture* aoRoughnessMetalnessTexture = nullptr;

   glm::vec4 albedo = glm::vec4(1.0f);
   glm::vec4 emissive = glm::vec4(0.0f);

   float emissiveIntensity = 0.0f;
   float roughness = 0.5f;
   float metalness = 0.0f;
   float ambientOcclusion = 1.0f;

   bool interpretAlphaAsMasked = false;
   bool twoSided = false;
};

class PhysicallyBasedMaterial : public Material
{
public:
   static const std::string kAlbedoTextureParameterName;
   static const std::string kNormalTextureParameterName;
   static const std::string kAoRoughnessMetalnessTextureParameterName;

   static const std::string kAlbedoVectorParameterName;
   static const std::string kEmissiveVectorParameterName;

   static const std::string kRoughnessScalarParameterName;
   static const std::string kMetalnessScalarParameterName;
   static const std::string kAmbientOcclusionScalarParameterName;

   static std::array<vk::DescriptorSetLayoutBinding, 4> getBindings();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, MaterialLoader& owningMaterialLoader, const PhysicallyBasedMaterialParams& materialParams);

   void update() override;

   const glm::vec4& getAlbedoColor() const
   {
      return cachedUniformData.albedo;
   }

   void setAlbedoColor(glm::vec4 albedo);

   const glm::vec4& getEmissiveColor() const
   {
      return cachedUniformData.emissive;
   }

   void setEmissiveColor(glm::vec4 emissive);

   float getEmissiveIntensity() const
   {
      return cachedUniformData.emissiveIntensity;
   }

   void setEmissiveIntensity(float emissiveIntensity);

   float getRoughness() const
   {
      return cachedUniformData.roughness;
   }

   void setRoughness(float roughness);

   float getMetalness() const
   {
      return cachedUniformData.metalness;
   }

   void setMetalness(float metalness);

   float getAmbientOcclusion() const
   {
      return cachedUniformData.ambientOcclusion;
   }

   void setAmbientOcclusion(float ambientOcclusion);

private:
   void onUniformDataChanged();

   UniformBuffer<PhysicallyBasedMaterialUniformData> uniformBuffer;
   PhysicallyBasedMaterialUniformData cachedUniformData;
};
