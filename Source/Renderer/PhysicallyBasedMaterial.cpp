#include "Renderer/PhysicallyBasedMaterial.h"

#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/Texture.h"

#include "Resources/MaterialResourceManager.h"

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
std::array<vk::DescriptorSetLayoutBinding, 4> PhysicallyBasedMaterial::getBindings()
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

// static
vk::DescriptorSetLayout PhysicallyBasedMaterial::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<PhysicallyBasedMaterial>(context);
}

PhysicallyBasedMaterial::PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, MaterialResourceManager& owningResourceManager, const PhysicallyBasedMaterialParams& materialParams)
   : Material(graphicsContext, owningResourceManager, DescriptorSetLayout::getCreateInfo<PhysicallyBasedMaterial>())
   , uniformBuffer(graphicsContext)
{
   ASSERT(materialParams.albedoTexture && materialParams.normalTexture && materialParams.aoRoughnessMetalnessTexture);

   cachedUniformData.albedo = materialParams.albedo;
   cachedUniformData.emissive = materialParams.emissive;
   cachedUniformData.emissiveIntensity = materialParams.emissiveIntensity;
   cachedUniformData.roughness = materialParams.roughness;
   cachedUniformData.metalness = materialParams.metalness;
   cachedUniformData.ambientOcclusion = materialParams.ambientOcclusion;
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
         .setSampler(materialResourceManager.getSampler());

      imageInfo[frameIndex * 3 + 1] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.normalTexture->getDefaultView())
         .setSampler(materialResourceManager.getSampler());

      imageInfo[frameIndex * 3 + 2] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.aoRoughnessMetalnessTexture->getDefaultView())
         .setSampler(materialResourceManager.getSampler());

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

void PhysicallyBasedMaterial::setAlbedoColor(const glm::vec4& albedo)
{
   if (cachedUniformData.albedo != albedo)
   {
      cachedUniformData.albedo = albedo;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setEmissiveColor(const glm::vec4& emissive)
{
   if (cachedUniformData.emissive != emissive)
   {
      cachedUniformData.emissive = emissive;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setEmissiveIntensity(float emissiveIntensity)
{
   if (cachedUniformData.emissiveIntensity != emissiveIntensity)
   {
      cachedUniformData.emissiveIntensity = emissiveIntensity;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setRoughness(float roughness)
{
   if (cachedUniformData.roughness != roughness)
   {
      cachedUniformData.roughness = roughness;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setMetalness(float metalness)
{
   if (cachedUniformData.metalness != metalness)
   {
      cachedUniformData.metalness = metalness;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::setAmbientOcclusion(float ambientOcclusion)
{
   if (cachedUniformData.ambientOcclusion != ambientOcclusion)
   {
      cachedUniformData.ambientOcclusion = ambientOcclusion;
      onUniformDataChanged();
   }
}

void PhysicallyBasedMaterial::onUniformDataChanged()
{
   materialResourceManager.requestSetOfUpdates(getHandle());
}
