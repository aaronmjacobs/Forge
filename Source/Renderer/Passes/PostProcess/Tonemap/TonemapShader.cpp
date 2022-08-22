#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct TonemapShaderStageData
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
            .setSize(sizeof(VkBool32))
      };

      std::array<std::array<VkBool32, 2>, 4> specializationData =
      { {
         { false, false },
         { false, true },
         { true, false },
         { true, true }
      } };

      std::array<vk::SpecializationInfo, 4> specializationInfo =
      {
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<std::array<VkBool32, 2>>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<std::array<VkBool32, 2>>(specializationData[1]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<std::array<VkBool32, 2>>(specializationData[2]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntries).setData<std::array<VkBool32, 2>>(specializationData[3])
      };
   };

   const TonemapShaderStageData& getStageData()
   {
      static const TonemapShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Tonemap.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 2> TonemapShader::getBindings()
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
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

// static
const vk::DescriptorSetLayoutCreateInfo& TonemapShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<TonemapShader>();
}

// static
vk::DescriptorSetLayout TonemapShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<TonemapShader>(context);
}

// static
uint32_t TonemapShader::getPermutationIndex(bool outputHDR, bool withBloom)
{
   // TODO Permutation based on color space instead
   return (outputHDR * 0b10) | (withBloom * 0b01);
}

TonemapShader::TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void TonemapShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> TonemapShader::getStages(bool outputHDR, bool withBloom) const
{
   return getStagesForPermutation(getPermutationIndex(outputHDR, withBloom));
}

std::vector<vk::DescriptorSetLayout> TonemapShader::getSetLayouts() const
{
   return { getLayout(context) };
}
