#include "Renderer/Passes/Composite/CompositeShader.h"

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   struct CompositeShaderStageData
   {
      vk::SpecializationMapEntry specializationMapEntry = vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0 * sizeof(int))
            .setSize(sizeof(int));

      std::array<std::array<int, 1>, CompositeShader::kNumModes> specializationData =
      { {
         { 0 },
         { 1 },
         { 2 }
      } };

      std::array<vk::SpecializationInfo, CompositeShader::kNumModes> specializationInfo =
      {
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<int>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<int>(specializationData[1]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<int>(specializationData[2])
      };
   };

   const CompositeShaderStageData& getStageData()
   {
      static const CompositeShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Composite.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> CompositeShader::getBindings()
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
const vk::DescriptorSetLayoutCreateInfo& CompositeShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<CompositeShader>();
}

// static
vk::DescriptorSetLayout CompositeShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<CompositeShader>(context);
}

CompositeShader::CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void CompositeShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> CompositeShader::getStages(Mode mode) const
{
   return getStagesForPermutation(Enum::cast(mode));
}

std::vector<vk::DescriptorSetLayout> CompositeShader::getSetLayouts() const
{
   return { getLayout(context) };
}
