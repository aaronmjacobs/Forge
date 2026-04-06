#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/SpecializationInfo.h"

namespace
{
   struct TonemapSpecializationValues
   {
      TonemappingAlgorithm tonemappingAlgorithm = TonemappingAlgorithm::None;
      ColorGamut colorGamut = ColorGamut::Rec709;
      TransferFunction transferFunction = TransferFunction::Linear;
      VkBool32 outputHDR = false;
      VkBool32 withBloom = false;
      VkBool32 withUI = false;
      VkBool32 showTestPattern = false;

      uint32_t getIndex() const
      {
         return (static_cast<uint32_t>(tonemappingAlgorithm) * 192) + (static_cast<uint32_t>(colorGamut) * 64) + (static_cast<uint32_t>(transferFunction) * 16) + (outputHDR * 8) + (withBloom * 4) + (withUI * 2) + (showTestPattern * 1);
      }
   };

   SpecializationInfo<TonemapSpecializationValues> createSpecializationInfo()
   {
      SpecializationInfoBuilder<TonemapSpecializationValues> builder;

      builder.registerMember(&TonemapSpecializationValues::tonemappingAlgorithm, TonemappingAlgorithm::None, TonemappingAlgorithm::DoubleFine);
      builder.registerMember(&TonemapSpecializationValues::colorGamut, ColorGamut::Rec709, ColorGamut::P3);
      builder.registerMember(&TonemapSpecializationValues::transferFunction, TransferFunction::Linear, TransferFunction::HybridLogGamma);
      builder.registerMember(&TonemapSpecializationValues::outputHDR);
      builder.registerMember(&TonemapSpecializationValues::withBloom);
      builder.registerMember(&TonemapSpecializationValues::withUI);
      builder.registerMember(&TonemapSpecializationValues::showTestPattern);

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
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(4)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

TonemapShader::TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> TonemapShader::getStages(TonemappingAlgorithm tonemappingAlgorithm, ColorGamut colorGamut, TransferFunction transferFunction,  bool outputHDR, bool withBloom, bool withUI, bool showTestPattern) const
{
   TonemapSpecializationValues specializationValues;
   specializationValues.tonemappingAlgorithm = tonemappingAlgorithm;
   specializationValues.colorGamut = colorGamut;
   specializationValues.transferFunction = transferFunction;
   specializationValues.outputHDR = outputHDR;
   specializationValues.withBloom = withBloom;
   specializationValues.withUI = withUI;
   specializationValues.showTestPattern = showTestPattern;

   return getStagesForPermutation(specializationValues.getIndex());
}
