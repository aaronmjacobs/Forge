#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"

#include "Core/Enum.h"

#include "Graphics/SpecializationInfo.h"

namespace
{
   struct BloomUpsampleSpecializationValues
   {
      RenderQuality quality = RenderQuality::Disabled;
      VkBool32 horizontal = false;

      uint32_t getIndex() const
      {
         return (static_cast<uint32_t>(quality) << 1) | (horizontal << 0);
      }
   };

   SpecializationInfo<BloomUpsampleSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<BloomUpsampleSpecializationValues> builder;

      builder.registerMember(&BloomUpsampleSpecializationValues::quality, RenderQuality::Disabled, RenderQuality::High);
      builder.registerMember(&BloomUpsampleSpecializationValues::horizontal);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModuleName = "Screen";
      info.fragShaderModuleName = "BloomUpsample";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::vector<vk::DescriptorSetLayoutBinding> BloomUpsampleDescriptorSet::getBindings()
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
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

BloomUpsampleShader::BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomUpsampleShader::getStages(RenderQuality quality, bool horizontal) const
{
   BloomUpsampleSpecializationValues specializationValues;
   specializationValues.quality = quality;
   specializationValues.horizontal = horizontal;

   return getStagesForPermutation(specializationValues.getIndex());
}
