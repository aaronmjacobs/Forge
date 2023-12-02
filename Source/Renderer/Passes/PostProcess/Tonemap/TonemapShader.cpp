#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/SpecializationInfo.h"

namespace
{
   struct TonemapSpecializationValues
   {
      VkBool32 outputHDR = false;
      VkBool32 withBloom = false;
      VkBool32 withUI = false;
      TonemappingAlgorithm tonemappingAlgorithm = TonemappingAlgorithm::None;

      uint32_t getIndex() const
      {
         return (outputHDR << 4) | (withBloom << 3) | (withUI << 2) | (static_cast<int32_t>(tonemappingAlgorithm) << 0);
      }
   };

   SpecializationInfo<TonemapSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<TonemapSpecializationValues> builder;

      builder.registerMember(&TonemapSpecializationValues::outputHDR);
      builder.registerMember(&TonemapSpecializationValues::withBloom);
      builder.registerMember(&TonemapSpecializationValues::withUI);
      builder.registerMember(&TonemapSpecializationValues::tonemappingAlgorithm, TonemappingAlgorithm::None, TonemappingAlgorithm::TonyMcMapface);

      return builder.build();
   }

   Shader::InitializationInfo getInitializationInfo()
   {
      static const SpecializationInfo kSpecializationInfo = createSpecializationInfo();

      Shader::InitializationInfo info;

      info.vertShaderModuleName = "Screen";
      info.fragShaderModuleName = "Tonemap";

      info.specializationInfo = kSpecializationInfo.getInfo();

      return info;
   }
}

// static
std::vector<vk::DescriptorSetLayoutBinding> TonemapDescriptorSet::getBindings()
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
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(2)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(3)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

TonemapShader::TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> TonemapShader::getStages(bool outputHDR, bool withBloom, bool withUI, TonemappingAlgorithm tonemappingAlgorithm) const
{
   TonemapSpecializationValues specializationValues;
   specializationValues.outputHDR = outputHDR;
   specializationValues.withBloom = withBloom;
   specializationValues.withUI = withUI;
   specializationValues.tonemappingAlgorithm = tonemappingAlgorithm;

   return getStagesForPermutation(specializationValues.getIndex());
}
