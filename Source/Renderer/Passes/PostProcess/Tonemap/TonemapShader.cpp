#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"
#include "Graphics/SpecializationInfo.h"

namespace
{
   struct TonemapSpecializationValues
   {
      VkBool32 outputHDR = false;
      VkBool32 withBloom = false;

      uint32_t getIndex() const
      {
         return outputHDR | (withBloom << 1);
      }
   };

   SpecializationInfo<TonemapSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<TonemapSpecializationValues> builder;

      builder.registerMember(&TonemapSpecializationValues::outputHDR);
      builder.registerMember(&TonemapSpecializationValues::withBloom);

      builder.addPermutation(TonemapSpecializationValues{ false, false });
      builder.addPermutation(TonemapSpecializationValues{ false, true });
      builder.addPermutation(TonemapSpecializationValues{ true, false });
      builder.addPermutation(TonemapSpecializationValues{ true, true });

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Tonemap.frag.spv";

      info.specializationInfo = kSpecializationInfo.getInfo();

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
   TonemapSpecializationValues specializationValues;
   specializationValues.outputHDR = outputHDR;
   specializationValues.withBloom = withBloom;

   return getStagesForPermutation(specializationValues.getIndex());
}

std::vector<vk::DescriptorSetLayout> TonemapShader::getSetLayouts() const
{
   return { getLayout(context) };
}
