#include "Renderer/PhysicallyBasedMaterial.h"

#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/Texture.h"

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

PhysicallyBasedMaterial::PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Sampler sampler, const PhysicallyBasedMaterialParams& materialParams)
   : Material(graphicsContext, dynamicDescriptorPool, DescriptorSetLayout::getCreateInfo<PhysicallyBasedMaterial>())
   , uniformBuffer(graphicsContext)
{
   ASSERT(materialParams.albedoTexture && materialParams.normalTexture && materialParams.aoRoughnessMetalnessTexture);

   PhysicallyBasedMaterialUniformData uniformData;
   uniformData.albedo = materialParams.albedo;
   uniformData.emissive = materialParams.emissive;
   uniformData.roughness = materialParams.roughness;
   uniformData.metalness = materialParams.metalness;
   uniformData.ambientOcclusion = materialParams.ambientOcclusion;
   uniformBuffer.updateAll(uniformData);

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
         .setSampler(sampler);

      imageInfo[frameIndex * 3 + 1] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.normalTexture->getDefaultView())
         .setSampler(sampler);

      imageInfo[frameIndex * 3 + 2] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(materialParams.aoRoughnessMetalnessTexture->getDefaultView())
         .setSampler(sampler);

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

void PhysicallyBasedMaterial::setAlbedoColor(const glm::vec4& albedo)
{
   uniformBuffer.updateAllMembers(&PhysicallyBasedMaterialUniformData::albedo, albedo); // TODO Only update current frame, but somehow queue up changes for the next two frames as well
}

void PhysicallyBasedMaterial::setEmissiveColor(const glm::vec4& emissive)
{
   uniformBuffer.updateAllMembers(&PhysicallyBasedMaterialUniformData::emissive, emissive); // TODO Only update current frame, but somehow queue up changes for the next two frames as well
}
