#include "Renderer/Passes/SSAO/SSAOBlurShader.h"

#include "Graphics/SpecializationInfo.h"

// static
void SSAOBlurShaderConstants::registerMembers(ShaderPermutationManager<SSAOBlurShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&SSAOBlurShaderConstants::horizontal);
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
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Screen", "SSAOBlur"))
{
}
