#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct TonemapShaderStageData
   {
      vk::SpecializationMapEntry specializationMapEntry = vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0 * sizeof(VkBool32))
            .setSize(sizeof(VkBool32));

      std::array<std::array<VkBool32, 1>, 2> specializationData =
      { {
         { false },
         { true }
      } };

      std::array<vk::SpecializationInfo, 2> specializationInfo =
      {
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<std::array<VkBool32, 1>>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<std::array<VkBool32, 1>>(specializationData[1])
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
uint32_t TonemapShader::getPermutationIndex(bool outputHDR)
{
   // TODO Permutation based on color space instead
   return outputHDR ? 1 : 0;
}

TonemapShader::TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void TonemapShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> TonemapShader::getStages(bool outputHDR) const
{
   return getStagesForPermutation(getPermutationIndex(outputHDR));
}

std::vector<vk::DescriptorSetLayout> TonemapShader::getSetLayouts() const
{
   return { getLayout(context) };
}
