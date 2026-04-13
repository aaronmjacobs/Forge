#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/ShaderPermutationManager.h"

// static
void TonemapShaderConstants::registerMembers(ShaderPermutationManager<TonemapShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&TonemapShaderConstants::tonemappingAlgorithm, TonemappingAlgorithm::None, TonemappingAlgorithm::DoubleFine);
   permutationManager.registerMember(&TonemapShaderConstants::colorGamut, ColorGamut::Rec709, ColorGamut::P3);
   permutationManager.registerMember(&TonemapShaderConstants::transferFunction, TransferFunction::Linear, TransferFunction::HybridLogGamma);
   permutationManager.registerMember(&TonemapShaderConstants::outputHDR);
   permutationManager.registerMember(&TonemapShaderConstants::withBloom);
   permutationManager.registerMember(&TonemapShaderConstants::withUI);
   permutationManager.registerMember(&TonemapShaderConstants::showTestPattern);
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
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Screen", "Tonemap"))
{
}
