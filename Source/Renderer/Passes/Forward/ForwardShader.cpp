#include "Renderer/Passes/Forward/ForwardShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

namespace
{
   struct ForwardShaderStageData
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

   const ForwardShaderStageData& getStageData()
   {
      static const ForwardShaderStageData kStageData;
      return kStageData;
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Forward.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Forward.frag.spv";

      info.specializationInfo = getStageData().specializationInfo;

      return info;
   }
}

uint32_t ForwardShader::getPermutationIndex(bool withTextures, bool withBlending)
{
   return (withTextures ? 0b10 : 0b00) | (withBlending ? 0b01 : 0b00);
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> ForwardShader::getBindings()
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
const vk::DescriptorSetLayoutCreateInfo& ForwardShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<ForwardShader>();
}

// static
vk::DescriptorSetLayout ForwardShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<ForwardShader>(context);
}

ForwardShader::ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void ForwardShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const DescriptorSet& descriptorSet, const ForwardLighting& lighting, const Material& material)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), descriptorSet.getCurrentSet(), lighting.getDescriptorSet().getCurrentSet(), material.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> ForwardShader::getStages(bool withTextures, bool withBlending) const
{
   return getStagesForPermutation(getPermutationIndex(withTextures, withBlending));
}

std::vector<vk::DescriptorSetLayout> ForwardShader::getSetLayouts() const
{
   return { View::getLayout(context), getLayout(context), ForwardLighting::getLayout(context), PhysicallyBasedMaterial::getLayout(context) };
}

std::vector<vk::PushConstantRange> ForwardShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
