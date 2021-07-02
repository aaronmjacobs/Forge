#include "MaterialResourceManager.h"

#include "Core/Hash.h"

#include "Renderer/PhongMaterial.h"

#include "Resources/ResourceManager.h"

#include <utility>

bool TextureMaterialParameter::operator==(const TextureMaterialParameter& other) const
{
   return name == other.name
      && value == other.value;
}

bool VectorMaterialParameter::operator==(const VectorMaterialParameter& other) const
{
   return name == other.name
      && value == other.value;
}

bool FloatMaterialParameter::operator==(const FloatMaterialParameter& other) const
{
   return name == other.name
      && value == other.value;
}

bool MaterialParameters::operator==(const MaterialParameters& other) const
{
   return textureParameters == other.textureParameters
      && vectorParameters == other.vectorParameters
      && scalarParameters == other.scalarParameters;
}

namespace std
{
   size_t hash<TextureMaterialParameter>::operator()(const TextureMaterialParameter& textureMaterialParameters) const
   {
      size_t hash = 0;

      Hash::combine(hash, textureMaterialParameters.name);
      Hash::combine(hash, textureMaterialParameters.value);

      return hash;
   }

   size_t hash<VectorMaterialParameter>::operator()(const VectorMaterialParameter& vectorMaterialParameters) const
   {
      size_t hash = 0;

      Hash::combine(hash, vectorMaterialParameters.name);
      Hash::combine(hash, vectorMaterialParameters.value);

      return hash;
   }

   size_t hash<FloatMaterialParameter>::operator()(const FloatMaterialParameter& floatMaterialParameters) const
   {
      size_t hash = 0;

      Hash::combine(hash, floatMaterialParameters.name);
      Hash::combine(hash, floatMaterialParameters.value);

      return hash;
   }

   size_t hash<MaterialParameters>::operator()(const MaterialParameters& materialParameters) const
   {
      size_t hash = 0;

      Hash::combine(hash, materialParameters.textureParameters);
      Hash::combine(hash, materialParameters.vectorParameters);
      Hash::combine(hash, materialParameters.scalarParameters);

      return hash;
   }
}

MaterialResourceManager::MaterialResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceManagerBase(graphicsContext, owningResourceManager)
{
   static const uint32_t kMaxImages = 100; // TODO
   static const uint32_t kMaxSets = 50; // TODO

   vk::DescriptorPoolSize samplerPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(kMaxImages * GraphicsContext::kMaxFramesInFlight);

   vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
      .setPoolSizes(samplerPoolSize)
      .setMaxSets(kMaxSets * GraphicsContext::kMaxFramesInFlight);
   descriptorPool = context.getDevice().createDescriptorPool(createInfo);

   bool anisotropySupported = context.getPhysicalDeviceFeatures().samplerAnisotropy;
   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
      .setAnisotropyEnable(anisotropySupported)
      .setMaxAnisotropy(anisotropySupported ? 16.0f : 1.0f)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(0.0f)
      .setMinLod(0.0f)
      .setMaxLod(16.0f);
   sampler = context.getDevice().createSampler(samplerCreateInfo);
}

MaterialResourceManager::~MaterialResourceManager()
{
   context.getDevice().destroySampler(sampler);
   context.getDevice().destroyDescriptorPool(descriptorPool);
}

MaterialHandle MaterialResourceManager::load(const MaterialParameters& parameters)
{
   if (std::optional<Handle> cachedHandle = getCachedHandle(parameters))
   {
      return *cachedHandle;
   }

   if (std::unique_ptr<Material> material = createMaterial(parameters))
   {
      MaterialHandle handle = addResource(std::move(material));
      cacheHandle(parameters, handle);

      return handle;
   }

   return MaterialHandle();
}

std::unique_ptr<Material> MaterialResourceManager::createMaterial(const MaterialParameters& parameters) const
{
   const Texture* diffuseTexture = nullptr;
   const Texture* normalTexture = nullptr;

   for (const TextureMaterialParameter& textureMaterialParameter : parameters.textureParameters)
   {
      if (textureMaterialParameter.name == PhongMaterial::kDiffuseTextureParameterName)
      {
         diffuseTexture = resourceManager.getTexture(textureMaterialParameter.value);
      }
      else if (textureMaterialParameter.name == PhongMaterial::kNormalTextureParameterName)
      {
         normalTexture = resourceManager.getTexture(textureMaterialParameter.value);
      }
   }

   if (diffuseTexture && normalTexture)
   {
      return std::make_unique<PhongMaterial>(context, descriptorPool, sampler, *diffuseTexture, *normalTexture);
   }

   return nullptr;
}
