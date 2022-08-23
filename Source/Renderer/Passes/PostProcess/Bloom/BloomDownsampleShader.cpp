#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct BloomDownsampleSpecializationValues
   {
      RenderQuality quality = RenderQuality::Disabled;

      uint32_t getIndex() const
      {
         return static_cast<uint32_t>(quality);
      }
   };

   SpecializationInfo<BloomDownsampleSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<BloomDownsampleSpecializationValues> builder;

      builder.registerMember(&BloomDownsampleSpecializationValues::quality);

      builder.addPermutation(BloomDownsampleSpecializationValues{ RenderQuality::Disabled });
      builder.addPermutation(BloomDownsampleSpecializationValues{ RenderQuality::Low });
      builder.addPermutation(BloomDownsampleSpecializationValues{ RenderQuality::Medium });
      builder.addPermutation(BloomDownsampleSpecializationValues{ RenderQuality::High });

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/BloomDownsample.frag.spv";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> BloomDownsampleShader::getBindings()
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

// static
const vk::DescriptorSetLayoutCreateInfo& BloomDownsampleShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<BloomDownsampleShader>();
}

// static
vk::DescriptorSetLayout BloomDownsampleShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<BloomDownsampleShader>(context);
}

BloomDownsampleShader::BloomDownsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void BloomDownsampleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomDownsampleShader::getStages(RenderQuality quality) const
{
   BloomDownsampleSpecializationValues specializationValues;
   specializationValues.quality = quality;

   return getStagesForPermutation(specializationValues.getIndex());
}

std::vector<vk::DescriptorSetLayout> BloomDownsampleShader::getSetLayouts() const
{
   return { getLayout(context) };
}
