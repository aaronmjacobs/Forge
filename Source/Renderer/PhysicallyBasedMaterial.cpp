#include "Renderer/PhysicallyBasedMaterial.h"

#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/Texture.h"

// static
std::array<vk::DescriptorSetLayoutBinding, 3> PhysicallyBasedMaterial::getBindings()
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
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

// static
vk::DescriptorSetLayout PhysicallyBasedMaterial::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<PhysicallyBasedMaterial>(context);
}

// static
const std::string PhysicallyBasedMaterial::kAlbedoTextureParameterName = "albedo";

// static
const std::string PhysicallyBasedMaterial::kNormalTextureParameterName = "normal";

// static
const std::string PhysicallyBasedMaterial::kAoRoughnessMetalnessTextureParameterName = "aoRoughnessMetalness";

PhysicallyBasedMaterial::PhysicallyBasedMaterial(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Sampler sampler, const Texture& albedoTexture, const Texture& normalTexture, const Texture& aoRoughnessMetalnessTexture, bool interpretAlphaAsMask)
   : Material(graphicsContext, dynamicDescriptorPool, DescriptorSetLayout::getCreateInfo<PhysicallyBasedMaterial>())
{
   if (albedoTexture.getImageProperties().hasAlpha)
   {
      blendMode = interpretAlphaAsMask ? BlendMode::Masked : BlendMode::Translucent;
   }

   std::array<vk::DescriptorImageInfo, GraphicsContext::kMaxFramesInFlight * 3> imageInfo;
   std::array<vk::WriteDescriptorSet, GraphicsContext::kMaxFramesInFlight * 3> descriptorWrites;
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      imageInfo[frameIndex * 3 + 0] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(albedoTexture.getDefaultView())
         .setSampler(sampler);

      imageInfo[frameIndex * 3 + 1] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(normalTexture.getDefaultView())
         .setSampler(sampler);

      imageInfo[frameIndex * 3 + 2] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(aoRoughnessMetalnessTexture.getDefaultView())
         .setSampler(sampler);

      descriptorWrites[frameIndex * 3 + 0] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 3 + 0]);

      descriptorWrites[frameIndex * 3 + 1] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 3 + 1]);

         descriptorWrites[frameIndex * 3 + 2] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(2)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 3 + 2]);
   }

   device.updateDescriptorSets(descriptorWrites, {});
}
