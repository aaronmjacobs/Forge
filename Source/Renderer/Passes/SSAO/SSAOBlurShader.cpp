#include "Renderer/Passes/SSAO/SSAOBlurShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/SpecializationInfo.h"

#include "Renderer/View.h"

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
   SSAOBlurSpecializationValues specializationValues;
   specializationValues.horizontal = horizontal;

   return getStagesForPermutation(specializationValues.getIndex());
}

std::vector<vk::DescriptorSetLayout> SSAOBlurShader::getSetLayouts() const
{
   return { View::getLayout(context), getLayout(context) };
}
