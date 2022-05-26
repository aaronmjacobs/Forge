#include "Renderer/Passes/Normal/NormalShader.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

namespace
{
   struct NormalShaderStageData
   {
      std::array<vk::SpecializationMapEntry, 2> specializationMapEntries =
      {
         vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0)
            .setSize(sizeof(VkBool32)),
         vk::SpecializationMapEntry()
            .setConstantID(1)
            .setOffset(1)
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

   const NormalShaderStageData& getStageData()
   {
      static const NormalShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Normal.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Normal.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

      return info;
   }
}

uint32_t NormalShader::getPermutationIndex(bool withTextures, bool masked)
{
   return (withTextures ? 0b10 : 0b00) | (masked ? 0b01 : 0b00);
}

NormalShader::NormalShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void NormalShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Material& material)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), material.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> NormalShader::getStages(bool withTextures, bool masked) const
{
   return getStagesForPermutation(getPermutationIndex(withTextures, masked));
}

std::vector<vk::DescriptorSetLayout> NormalShader::getSetLayouts() const
{
   return { View::getLayout(context), PhysicallyBasedMaterial::getLayout(context) };
}

std::vector<vk::PushConstantRange> NormalShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
