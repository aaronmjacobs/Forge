#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct BloomUpsampleSpecializationValues
   {
      RenderQuality quality = RenderQuality::Disabled;
      VkBool32 horizontal = false;

      uint32_t getIndex() const
      {
         return horizontal | (static_cast<uint32_t>(quality) << 1);
      }
   };

   SpecializationInfo<BloomUpsampleSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<BloomUpsampleSpecializationValues> builder;

      builder.registerMember(&BloomUpsampleSpecializationValues::quality);
      builder.registerMember(&BloomUpsampleSpecializationValues::horizontal);

      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::Disabled, false });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::Disabled, true });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::Low, false });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::Low, true });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::Medium, false });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::Medium, true });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::High, false });
      builder.addPermutation(BloomUpsampleSpecializationValues{ RenderQuality::High, true });

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/BloomUpsample.frag.spv";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 3> BloomUpsampleShader::getBindings()
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

// static
const vk::DescriptorSetLayoutCreateInfo& BloomUpsampleShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<BloomUpsampleShader>();
}

// static
vk::DescriptorSetLayout BloomUpsampleShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<BloomUpsampleShader>(context);
}

BloomUpsampleShader::BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void BloomUpsampleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomUpsampleShader::getStages(RenderQuality quality, bool horizontal) const
{
   BloomUpsampleSpecializationValues specializationValues;
   specializationValues.quality = quality;
   specializationValues.horizontal = horizontal;

   return getStagesForPermutation(specializationValues.getIndex());
}

std::vector<vk::DescriptorSetLayout> BloomUpsampleShader::getSetLayouts() const
{
   return { getLayout(context) };
}
