#include "MaterialResourceManager.h"

#include "Graphics/DebugUtils.h"

#include "Renderer/PhysicallyBasedMaterial.h"

#include "Resources/ResourceManager.h"

#include <utility>

namespace
{
   DynamicDescriptorPool::Sizes getDynamicDescriptorPoolSizes()
   {
      DynamicDescriptorPool::Sizes sizes;

      sizes.maxSets = 50;

      sizes.combinedImageSamplerCount = 100;
      sizes.uniformBufferCount = 100;

      return sizes;
   }
}

std::size_t TextureMaterialParameter::hash() const
{
   std::size_t hash = 0;

   Hash::combine(hash, name);
   Hash::combine(hash, value);
   Hash::combine(hash, interpretAlphaAsMask);

   return hash;
}

std::size_t VectorMaterialParameter::hash() const
{
   std::size_t hash = 0;

   Hash::combine(hash, name);
   Hash::combine(hash, value);

   return hash;
}

std::size_t ScalarMaterialParameter::hash() const
{
   std::size_t hash = 0;

   Hash::combine(hash, name);
   Hash::combine(hash, value);

   return hash;
}

std::size_t MaterialParameters::hash() const
{
   std::size_t hash = 0;

   Hash::combine(hash, textureParameters);
   Hash::combine(hash, vectorParameters);
   Hash::combine(hash, scalarParameters);
   Hash::combine(hash, twoSided);

   return hash;
}

MaterialResourceManager::MaterialResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceManagerBase(graphicsContext, owningResourceManager)
   , dynamicDescriptorPool(graphicsContext, getDynamicDescriptorPoolSizes())
{
   NAME_ITEM(context.getDevice(), dynamicDescriptorPool, "Material Resource Manager Dynamic Descriptor Pool");

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
   NAME_ITEM(context.getDevice(), sampler, "Default Material Sampler");
}

MaterialResourceManager::~MaterialResourceManager()
{
   context.getDevice().destroySampler(sampler);
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

std::unique_ptr<Material> MaterialResourceManager::createMaterial(const MaterialParameters& parameters)
{
   PhysicallyBasedMaterialParams pbrParams;

   for (const TextureMaterialParameter& textureMaterialParameter : parameters.textureParameters)
   {
      if (textureMaterialParameter.name == PhysicallyBasedMaterial::kAlbedoTextureParameterName)
      {
         pbrParams.albedoTexture = resourceManager.getTexture(textureMaterialParameter.value);
         pbrParams.interpretAlphaAsMasked = textureMaterialParameter.interpretAlphaAsMask;
      }
      else if (textureMaterialParameter.name == PhysicallyBasedMaterial::kNormalTextureParameterName)
      {
         pbrParams.normalTexture = resourceManager.getTexture(textureMaterialParameter.value);
      }
      else if (textureMaterialParameter.name == PhysicallyBasedMaterial::kAoRoughnessMetalnessTextureParameterName)
      {
         pbrParams.aoRoughnessMetalnessTexture = resourceManager.getTexture(textureMaterialParameter.value);
      }
   }

   for (const VectorMaterialParameter& vectorMaterialParameter : parameters.vectorParameters)
   {
      if (vectorMaterialParameter.name == PhysicallyBasedMaterial::kAlbedoVectorParameterName)
      {
         pbrParams.albedo = vectorMaterialParameter.value;
      }
      else if (vectorMaterialParameter.name == PhysicallyBasedMaterial::kEmissiveVectorParameterName)
      {
         pbrParams.emissive = vectorMaterialParameter.value;
      }
   }

   for (const ScalarMaterialParameter& scalarMaterialParameter : parameters.scalarParameters)
   {
      if (scalarMaterialParameter.name == PhysicallyBasedMaterial::kAmbientOcclusionScalarParameterName)
      {
         pbrParams.ambientOcclusion = scalarMaterialParameter.value;
      }
      else if (scalarMaterialParameter.name == PhysicallyBasedMaterial::kRoughnessScalarParameterName)
      {
         pbrParams.roughness = scalarMaterialParameter.value;
      }
      else if (scalarMaterialParameter.name == PhysicallyBasedMaterial::kMetalnessScalarParameterName)
      {
         pbrParams.metalness = scalarMaterialParameter.value;
      }
   }

   pbrParams.twoSided = parameters.twoSided;

   if (pbrParams.albedoTexture && pbrParams.normalTexture && pbrParams.aoRoughnessMetalnessTexture)
   {
      std::unique_ptr<PhysicallyBasedMaterial> material = std::make_unique<PhysicallyBasedMaterial>(context, dynamicDescriptorPool, sampler, pbrParams);
      NAME_POINTER(context.getDevice(), material, "Physically Based Material (Albedo = " + pbrParams.albedoTexture->getName() + ", Normal = " + pbrParams.normalTexture->getName() + ", Ambient Occlusion / Roughness / Metalness = " + pbrParams.aoRoughnessMetalnessTexture->getName() + ")");

      return material;
   }

   return nullptr;
}
