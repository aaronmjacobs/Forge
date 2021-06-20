#include "Renderer/PhongMaterial.h"

#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/Texture.h"

namespace
{
   const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo()
   {
      static const std::array<vk::DescriptorSetLayoutBinding, 2> kBindings =
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
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
      };

      static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), kBindings);

      return kCreateInfo;
   }
}

// static
vk::DescriptorSetLayout PhongMaterial::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

// static
const std::string PhongMaterial::kDiffuseTextureParameterName = "diffuse";

// static
const std::string PhongMaterial::kNormalTextureParameterName = "normal";

PhongMaterial::PhongMaterial(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, vk::Sampler sampler, const Texture& diffuseTexture, const Texture& normalTexture)
   : Material(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
   std::array<vk::DescriptorImageInfo, GraphicsContext::kMaxFramesInFlight * 2> imageInfo;
   std::array<vk::WriteDescriptorSet, GraphicsContext::kMaxFramesInFlight * 2> descriptorWrites;
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      imageInfo[frameIndex * 2 + 0] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(diffuseTexture.getDefaultView())
         .setSampler(sampler);

      imageInfo[frameIndex * 2 + 1] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(normalTexture.getDefaultView())
         .setSampler(sampler);

      descriptorWrites[frameIndex * 2 + 0] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 2 + 0]);

      descriptorWrites[frameIndex * 2 + 1] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex * 2 + 1]);
   }

   device.updateDescriptorSets(descriptorWrites, {});
}
