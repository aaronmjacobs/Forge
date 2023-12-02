#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"

#include "Graphics/SpecializationInfo.h"

namespace
{
   struct BloomDownsampleSpecializationValues
   {
      RenderQuality quality = RenderQuality::Disabled;

      uint32_t getIndex() const
      {
         return (static_cast<uint32_t>(quality) << 0);
      }
   };

   SpecializationInfo<BloomDownsampleSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<BloomDownsampleSpecializationValues> builder;

      builder.registerMember(&BloomDownsampleSpecializationValues::quality, RenderQuality::Disabled, RenderQuality::High);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModuleName = "Screen";
      info.fragShaderModuleName = "BloomDownsample";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::vector<vk::DescriptorSetLayoutBinding> BloomDownsampleDescriptorSet::getBindings()
{
   return
   {
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

BloomDownsampleShader::BloomDownsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomDownsampleShader::getStages(RenderQuality quality) const
{
   BloomDownsampleSpecializationValues specializationValues;
   specializationValues.quality = quality;

   return getStagesForPermutation(specializationValues.getIndex());
}
