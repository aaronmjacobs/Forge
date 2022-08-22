#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct BloomUpsampleSpecializationValues
   {
      VkBool32 horizontal = true;
      RenderQuality quality = RenderQuality::High;
   };

   struct BloomUpsampleShaderStageData
   {
      std::array<vk::SpecializationMapEntry, 2> specializationMapEntries =
      {
         vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0 * sizeof(VkBool32))
            .setSize(sizeof(VkBool32)),
         vk::SpecializationMapEntry()
            .setConstantID(1)
            .setOffset(1 * sizeof(VkBool32))
            .setSize(sizeof(RenderQuality))
      };

      std::array<BloomUpsampleSpecializationValues, 8> specializationData =
      {
         BloomUpsampleSpecializationValues{ false, RenderQuality::Disabled },
         BloomUpsampleSpecializationValues{ false, RenderQuality::Low },
         BloomUpsampleSpecializationValues{ false, RenderQuality::Medium },
         BloomUpsampleSpecializationValues{ false, RenderQuality::High },
         BloomUpsampleSpecializationValues{ true, RenderQuality::Disabled },
         BloomUpsampleSpecializationValues{ true, RenderQuality::Low },
         BloomUpsampleSpecializationValues{ true, RenderQuality::Medium },
         BloomUpsampleSpecializationValues{ true, RenderQuality::High },
      };

      std::array<vk::SpecializationInfo, 8> specializationInfo =
      {
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[1]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[2]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[3]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[4]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[5]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[6]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<BloomUpsampleSpecializationValues>(specializationData[7]),
      };
   };

   const BloomUpsampleShaderStageData& getStageData()
   {
      static const BloomUpsampleShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/BloomUpsample.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

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

// static
uint32_t BloomUpsampleShader::getPermutationIndex(bool horizontal, RenderQuality quality)
{
   return static_cast<uint32_t>(quality) + (Enum::cast(RenderQuality::High) + 1) * horizontal;
}

BloomUpsampleShader::BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void BloomUpsampleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomUpsampleShader::getStages(bool horizontal, RenderQuality quality) const
{
   return getStagesForPermutation(getPermutationIndex(horizontal, quality));
}

std::vector<vk::DescriptorSetLayout> BloomUpsampleShader::getSetLayouts() const
{
   return { getLayout(context) };
}
