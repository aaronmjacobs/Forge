#include "Renderer/PhysicallyBasedMaterial.h"

#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

#include "Resources/ResourceManager.h"

#include <vector>

// static
const std::string PhysicallyBasedMaterial::kAlbedoTextureParameterName = "albedo";

// static
const std::string PhysicallyBasedMaterial::kNormalTextureParameterName = "normal";

// static
const std::string PhysicallyBasedMaterial::kAoRoughnessMetalnessTextureParameterName = "aoRoughnessMetalness";

// static
const std::string PhysicallyBasedMaterial::kAlbedoVectorParameterName = "albedo";

// static
const std::string PhysicallyBasedMaterial::kEmissiveVectorParameterName = "emissive";

// static
const std::string PhysicallyBasedMaterial::kRoughnessScalarParameterName = "roughness";

// static
const std::string PhysicallyBasedMaterial::kMetalnessScalarParameterName = "metalness";

// static
const std::string PhysicallyBasedMaterial::kAmbientOcclusionScalarParameterName = "ambientOcclusion";

// static
std::vector<vk::DescriptorSetLayoutBinding> PhysicallyBasedMaterialDescriptorSet::getBindings()
{
   return
   {
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(1)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(2)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(3)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

PhysicallyBasedMaterial::PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager, DynamicDescriptorPool& dynamicDescriptorPool, vk::Sampler materialSampler, const PhysicallyBasedMaterialParams& materialParams)
   : Material(graphicsContext, owningResourceManager, kTypeFlag)
   , descriptorSet(graphicsContext, dynamicDescriptorPool)
   , uniformBuffer(graphicsContext)
   , sampler(materialSampler)
   , albedoTextureHandle(materialParams.albedoTexture)
   , normalTextureHandle(materialParams.normalTexture)
   , aoRoughnessMetalnessTextureHandle(materialParams.aoRoughnessMetalnessTexture)
   , interpretAlphaAsMasked(materialParams.interpretAlphaAsMasked)
{
   NAME_CHILD(descriptorSet, "");
   NAME_CHILD(uniformBuffer, "");

   cachedUniformData.albedo = MathUtils::saturate(materialParams.albedo);
   cachedUniformData.emissive = MathUtils::saturate(materialParams.emissive);
   cachedUniformData.emissiveIntensity = glm::max(materialParams.emissiveIntensity, 0.0f);
   cachedUniformData.roughness = MathUtils::saturate(materialParams.roughness);
   cachedUniformData.metalness = MathUtils::saturate(materialParams.metalness);
   cachedUniformData.ambientOcclusion = MathUtils::saturate(materialParams.ambientOcclusion);
   uniformBuffer.updateAll(cachedUniformData);

   twoSided = materialParams.twoSided;

   updateDescriptorSet(true, true, true, true);

   auto replaceLambda = [this](TextureHandle handle)
   {
      updateDescriptorSet(handle == albedoTextureHandle, handle == normalTextureHandle, handle == aoRoughnessMetalnessTextureHandle, false);
   };

   resourceManager.registerTextureReplaceDelegate(albedoTextureHandle, TextureLoader::ReplaceDelegate::create(replaceLambda));
   resourceManager.registerTextureReplaceDelegate(normalTextureHandle, TextureLoader::ReplaceDelegate::create(replaceLambda));
   resourceManager.registerTextureReplaceDelegate(aoRoughnessMetalnessTextureHandle, TextureLoader::ReplaceDelegate::create(replaceLambda));
}

PhysicallyBasedMaterial::~PhysicallyBasedMaterial()
{
   resourceManager.unregisterTextureReplaceDelegate(albedoTextureHandle);
   resourceManager.unregisterTextureReplaceDelegate(normalTextureHandle);
   resourceManager.unregisterTextureReplaceDelegate(aoRoughnessMetalnessTextureHandle);
}

void PhysicallyBasedMaterial::update()
{
   Material::update();

   uniformBuffer.update(cachedUniformData);
}

void PhysicallyBasedMaterial::setAlbedoColor(glm::vec4 albedo)
{
   albedo = MathUtils::saturate(albedo);

   if (cachedUniformData.albedo != albedo)
   {
      cachedUniformData.albedo = albedo;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setEmissiveColor(glm::vec4 emissive)
{
   emissive = MathUtils::saturate(emissive);

   if (cachedUniformData.emissive != emissive)
   {
      cachedUniformData.emissive = emissive;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setEmissiveIntensity(float emissiveIntensity)
{
   emissiveIntensity = glm::max(emissiveIntensity, 0.0f);

   if (cachedUniformData.emissiveIntensity != emissiveIntensity)
   {
      cachedUniformData.emissiveIntensity = emissiveIntensity;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setRoughness(float roughness)
{
   roughness = MathUtils::saturate(roughness);

   if (cachedUniformData.roughness != roughness)
   {
      cachedUniformData.roughness = roughness;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setMetalness(float metalness)
{
   metalness = MathUtils::saturate(metalness);

   if (cachedUniformData.metalness != metalness)
   {
      cachedUniformData.metalness = metalness;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setAmbientOcclusion(float ambientOcclusion)
{
   ambientOcclusion = MathUtils::saturate(ambientOcclusion);

   if (cachedUniformData.ambientOcclusion != ambientOcclusion)
   {
      cachedUniformData.ambientOcclusion = ambientOcclusion;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::onUniformDataChanged()
{
   resourceManager.requestSetOfMaterialUpdates(getHandle());
}

void PhysicallyBasedMaterial::updateDescriptorSet(bool updateAlbedo, bool updateNormal, bool updateAoRoughnessMetalness, bool updateUniformBuffer)
{
   Texture* albedoTexture = updateAlbedo ? resourceManager.getTexture(albedoTextureHandle) : nullptr;
   Texture* normalTexture = updateNormal ? resourceManager.getTexture(normalTextureHandle) : nullptr;
   Texture* aoRoughnessMetalnessTexture = updateAoRoughnessMetalness ? resourceManager.getTexture(aoRoughnessMetalnessTextureHandle) : nullptr;

   if (albedoTexture && albedoTexture->getImageProperties().hasAlpha)
   {
      blendMode = interpretAlphaAsMasked ? BlendMode::Masked : BlendMode::Translucent;
   }

   std::vector<vk::DescriptorImageInfo> imageInfo;
   if (updateAlbedo)
   {
      imageInfo.push_back(vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(albedoTexture->getDefaultView())
         .setSampler(sampler));
   }
   if (updateNormal)
   {
      imageInfo.push_back(vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(normalTexture->getDefaultView())
         .setSampler(sampler));
   }
   if (updateAoRoughnessMetalness)
   {
      imageInfo.push_back(vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(aoRoughnessMetalnessTexture->getDefaultView())
         .setSampler(sampler));
   }

   std::array<vk::DescriptorBufferInfo, 3> bufferInfo;
   std::vector<vk::WriteDescriptorSet> descriptorWrites;
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      uint32_t imageIndex = 0;

      if (updateAlbedo)
      {
         descriptorWrites.push_back(vk::WriteDescriptorSet()
            .setDstSet(descriptorSet.getSet(frameIndex))
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&imageInfo[imageIndex]));

         ++imageIndex;
      }

      if (updateNormal)
      {
         descriptorWrites.push_back(vk::WriteDescriptorSet()
            .setDstSet(descriptorSet.getSet(frameIndex))
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&imageInfo[imageIndex]));

         ++imageIndex;
      }

      if (updateAoRoughnessMetalness)
      {
         descriptorWrites.push_back(vk::WriteDescriptorSet()
            .setDstSet(descriptorSet.getSet(frameIndex))
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&imageInfo[imageIndex]));

         ++imageIndex;
      }

      if (updateUniformBuffer)
      {
         bufferInfo[frameIndex] = uniformBuffer.getDescriptorBufferInfo(frameIndex);
         descriptorWrites.push_back(vk::WriteDescriptorSet()
            .setDstSet(descriptorSet.getSet(frameIndex))
            .setDstBinding(3)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setBufferInfo(bufferInfo[frameIndex]));
      }
   }

   device.updateDescriptorSets(descriptorWrites, {});
}
