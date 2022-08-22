#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct BloomDownsampleShaderStageData
   {
      std::array<vk::SpecializationMapEntry, 1> specializationMapEntries =
      {
         vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0 * sizeof(RenderQuality))
            .setSize(sizeof(RenderQuality))
      };

      std::array<RenderQuality, 4> specializationData =
      {
         RenderQuality::Disabled,
         RenderQuality::Low,
         RenderQuality::Medium,
         RenderQuality::High
      };

      std::array<vk::SpecializationInfo, 4> specializationInfo =
      {
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<RenderQuality>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<RenderQuality>(specializationData[1]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<RenderQuality>(specializationData[2]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<RenderQuality>(specializationData[3])
      };
   };

   const BloomDownsampleShaderStageData& getStageData()
   {
      static const BloomDownsampleShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/BloomDownsample.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

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

// static
uint32_t BloomDownsampleShader::getPermutationIndex(RenderQuality quality)
{
   return static_cast<uint32_t>(quality);
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
   return getStagesForPermutation(getPermutationIndex(quality));
}

std::vector<vk::DescriptorSetLayout> BloomDownsampleShader::getSetLayouts() const
{
   return { getLayout(context) };
}
