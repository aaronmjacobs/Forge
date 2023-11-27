#include "Renderer/Passes/SSAO/SSAOBlurShader.h"

#include "Graphics/SpecializationInfo.h"

namespace
{
   struct SSAOBlurSpecializationValues
   {
      VkBool32 horizontal = false;

      uint32_t getIndex() const
      {
         return (horizontal << 0);
      }
   };

   SpecializationInfo<SSAOBlurSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<SSAOBlurSpecializationValues> builder;

      builder.registerMember(&SSAOBlurSpecializationValues::horizontal);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/SSAOBlur.frag.spv";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::vector<vk::DescriptorSetLayoutBinding> SSAOBlurDescriptorSet::getBindings()
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

SSAOBlurShader::SSAOBlurShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> SSAOBlurShader::getStages(bool horizontal) const
{
   SSAOBlurSpecializationValues specializationValues;
   specializationValues.horizontal = horizontal;

   return getStagesForPermutation(specializationValues.getIndex());
}
