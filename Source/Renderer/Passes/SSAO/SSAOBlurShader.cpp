#include "Renderer/Passes/SSAO/SSAOBlurShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

#include "Renderer/View.h"

namespace
{
   struct SSAOBlurShaderStageData
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
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<VkBool32>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<VkBool32>(specializationData[1])
      };
   };

   const SSAOBlurShaderStageData& getStageData()
   {
      static const SSAOBlurShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/SSAOBlur.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 2> SSAOBlurShader::getBindings()
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
const vk::DescriptorSetLayoutCreateInfo& SSAOBlurShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<SSAOBlurShader>();
}

// static
vk::DescriptorSetLayout SSAOBlurShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<SSAOBlurShader>(context);
}

SSAOBlurShader::SSAOBlurShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void SSAOBlurShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> SSAOBlurShader::getStages(bool horizontal) const
{
   return getStagesForPermutation(horizontal ? 1 : 0);
}

std::vector<vk::DescriptorSetLayout> SSAOBlurShader::getSetLayouts() const
{
   return { View::getLayout(context), getLayout(context) };
}
