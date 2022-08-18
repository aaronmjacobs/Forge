#pragma once

#include "Graphics/Material.h"
#include "Graphics/UniformBuffer.h"

#include <glm/glm.hpp>

class Texture;

struct PhysicallyBasedMaterialUniformData
{
   alignas(16) glm::vec4 albedo = glm::vec4(1.0f);
   alignas(16) glm::vec4 emissive = glm::vec4(0.0f);
   alignas(4) float roughness = 0.5f;
   alignas(4) float metalness = 0.0f;
   alignas(4) float ambientOcclusion = 1.0f;
};

class PhysicallyBasedMaterial : public Material
{
public:
   static std::array<vk::DescriptorSetLayoutBinding, 4> getBindings();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   static const std::string kAlbedoTextureParameterName;
   static const std::string kNormalTextureParameterName;
   static const std::string kAoRoughnessMetalnessTextureParameterName;

   PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Sampler sampler, const Texture& albedoTexture, const Texture& normalTexture, const Texture& aoRoughnessMetalnessTexture, bool interpretAlphaAsMask = false, bool twoSides = false);

   void setEmissiveColor(const glm::vec4& emissive);

private:
   UniformBuffer<PhysicallyBasedMaterialUniformData> uniformBuffer;
};
