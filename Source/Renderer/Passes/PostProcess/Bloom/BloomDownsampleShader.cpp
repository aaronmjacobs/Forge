#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"

#include "Graphics/SpecializationInfo.h"

// static
void BloomDownsampleShaderConstants::registerMembers(ShaderPermutationManager<BloomDownsampleShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&BloomDownsampleShaderConstants::quality, RenderQuality::Disabled, RenderQuality::High);
}

// static
std::vector<vk::DescriptorSetLayoutBinding> BloomDownsampleDescriptorSet::getBindings()
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

BloomDownsampleShader::BloomDownsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Screen", "BloomDownsample"))
{
}
