#include "Renderer/PhysicallyBasedMaterial.h"

#include "Graphics/Texture.h"

#include "Math/MathUtils.h"

#include "Resources/MaterialLoader.h"

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

PhysicallyBasedMaterial::PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, MaterialLoader& owningMaterialLoader, const PhysicallyBasedMaterialParams& materialParams)
   : Material(graphicsContext, owningMaterialLoader, kTypeFlag)
   , descriptorSet(graphicsContext, owningMaterialLoader.getDynamicDescriptorPool())
   , uniformBuffer(graphicsContext)
{
   ASSERT(materialParams.albedoTexture && materialParams.normalTexture && materialParams.aoRoughnessMetalnessTexture);

   NAME_CHILD(descriptorSet, "");
   NAME_CHILD(uniformBuffer, "");

   cachedUniformData.albedo = MathUtils::saturate(materialParams.albedo);
   cachedUniformData.emissive = MathUtils::saturate(materialParams.emissive);
   cachedUniformData.emissiveIntensity = glm::max(materialParams.emissiveIntensity, 0.0f);
   cachedUniformData.roughness = MathUtils::saturate(materialParams.roughness);
   cachedUniformData.metalness = MathUtils::saturate(materialParams.metalness);
   cachedUniformData.ambientOcclusion = MathUtils::saturate(materialParams.ambientOcclusion);
   uniformBuffer.updateAll(cachedUniformData);

   if (materialParams.albedoTexture->getImageProperties().hasAlpha)
   {
      blendMode = materialParams.interpretAlphaAsMasked ? BlendMode::Masked : BlendMode::Translucent;
   }

   twoSided = materialParams.twoSided;

   std::array<vk::DescriptorImageInfo, GraphicsContext::kMaxFramesInFlight * 3> imageInfo;
   std::array<vk::DescriptorBufferInfo, GraphicsContext::kMaxFramesInFlight> bufferInfo;
   std::array<vk::WriteDescriptorSet, GraphicsContext::kMaxFramesInFlight * 4> descriptorWrites;
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      imageInfo[frameIndex * 3 + 0] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.albedoTexture->getDefaultView())
         .setSampler(materialLoader.getSampler());

      imageInfo[frameIndex * 3 + 1] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.normalTexture->getDefaultView())
         .setSampler(materialLoader.getSampler());

      imageInfo[frameIndex * 3 + 2] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.aoRoughnessMetalnessTexture->getDefaultView())
         .setSampler(materialLoader.getSampler());

      descriptorWrites[frameIndex * 4 + 0] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 3 + 0]);

      descriptorWrites[frameIndex * 4 + 1] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 3 + 1]);

      descriptorWrites[frameIndex * 4 + 2] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(2)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 3 + 2]);

      bufferInfo[frameIndex] = uniformBuffer.getDescriptorBufferInfo(frameIndex);
      descriptorWrites[frameIndex * 4 + 3] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(3)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setBufferInfo(bufferInfo[frameIndex]);
   }

   device.updateDescriptorSets(descriptorWrites, {});
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
   materialLoader.requestSetOfUpdates(getHandle());
}
