#pragma once

#include "Graphics/Material.h"

class Texture;

class PhysicallyBasedMaterial : public Material
{
public:
   static std::array<vk::DescriptorSetLayoutBinding, 3> getBindings();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   static const std::string kAlbedoTextureParameterName;
   static const std::string kNormalTextureParameterName;
   static const std::string kAoRoughnessMetalnessTextureParameterName;

   PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Sampler sampler, const Texture& albedoTexture, const Texture& normalTexture, const Texture& aoRoughnessMetalnessTexture, bool interpretAlphaAsMask = false, bool twoSides = false);
};
