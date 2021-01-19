#include "Renderer/SimpleMaterial.h"

#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/Texture.h"

namespace
{
   const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo()
   {
      static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment);

      static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &kBinding);

      return kCreateInfo;
   }
}

// static
vk::DescriptorSetLayout SimpleMaterial::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

SimpleMaterial::SimpleMaterial(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, const Texture& texture, vk::Sampler sampler)
   : Material(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
   std::array<vk::DescriptorImageInfo, GraphicsContext::kMaxFramesInFlight> imageInfo;
   std::array<vk::WriteDescriptorSet, GraphicsContext::kMaxFramesInFlight> descriptorWrites;
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      imageInfo[frameIndex] = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(texture.getDefaultView())
         .setSampler(sampler);

      descriptorWrites[frameIndex] = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo[frameIndex]);
   }

   device.updateDescriptorSets(descriptorWrites, {});
}
