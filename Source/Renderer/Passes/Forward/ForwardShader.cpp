#include "Renderer/Passes/Forward/ForwardShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/SpecializationInfo.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

namespace
{
   struct ForwardSpecializationValues
   {
      VkBool32 withTextures = false;
      VkBool32 withBlending = false;

      uint32_t getIndex() const
      {
         return (withTextures << 1) | (withBlending << 0);
      }
   };

   SpecializationInfo<ForwardSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<ForwardSpecializationValues> builder;

      builder.registerMember(&ForwardSpecializationValues::withTextures);
      builder.registerMember(&ForwardSpecializationValues::withBlending);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Forward.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Forward.frag.spv";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 2> ForwardShader::getBindings()
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
   ForwardSpecializationValues specializationValues;
   specializationValues.withTextures = withTextures;
   specializationValues.withBlending = withBlending;

   return getStagesForPermutation(specializationValues.getIndex());
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
