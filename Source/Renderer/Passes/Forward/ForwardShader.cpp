#include "Renderer/Passes/Forward/ForwardShader.h"

#include "Graphics/SpecializationInfo.h"

#include "Renderer/UniformData.h"

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

      info.vertShaderModuleName = "Forward";
      info.fragShaderModuleName = "Forward";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::vector<vk::DescriptorSetLayoutBinding> ForwardDescriptorSet::getBindings()
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

ForwardShader::ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> ForwardShader::getStages(bool withTextures, bool withBlending) const
{
   ForwardSpecializationValues specializationValues;
   specializationValues.withTextures = withTextures;
   specializationValues.withBlending = withBlending;

   return getStagesForPermutation(specializationValues.getIndex());
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
