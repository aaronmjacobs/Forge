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

std::size_t FloatMaterialParameter::hash() const
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
   const Texture* albedoTexture = nullptr;
   const Texture* normalTexture = nullptr;
   const Texture* aoRoughnessMetalnessTexture = nullptr;
   bool interpretAlphaAsMask = false;

   for (const TextureMaterialParameter& textureMaterialParameter : parameters.textureParameters)
   {
      if (textureMaterialParameter.name == PhysicallyBasedMaterial::kAlbedoTextureParameterName)
      {
         albedoTexture = resourceManager.getTexture(textureMaterialParameter.value);
         interpretAlphaAsMask = textureMaterialParameter.interpretAlphaAsMask;
      }
      else if (textureMaterialParameter.name == PhysicallyBasedMaterial::kNormalTextureParameterName)
      {
         normalTexture = resourceManager.getTexture(textureMaterialParameter.value);
      }
      else if (textureMaterialParameter.name == PhysicallyBasedMaterial::kAoRoughnessMetalnessTextureParameterName)
      {
         aoRoughnessMetalnessTexture = resourceManager.getTexture(textureMaterialParameter.value);
      }
   }

   if (albedoTexture && normalTexture && aoRoughnessMetalnessTexture)
   {
      std::unique_ptr<Material> material = std::make_unique<PhysicallyBasedMaterial>(context, dynamicDescriptorPool, sampler, *albedoTexture, *normalTexture, *aoRoughnessMetalnessTexture, interpretAlphaAsMask, parameters.twoSided);
      NAME_POINTER(context.getDevice(), material, "Physically Based Material (Albedo = " + albedoTexture->getName() + ", Normal = " + normalTexture->getName() + ", Ambient Occlusion / Roughness / Metalness = " + aoRoughnessMetalnessTexture->getName() + ")");

      return material;
   }

   return nullptr;
}
