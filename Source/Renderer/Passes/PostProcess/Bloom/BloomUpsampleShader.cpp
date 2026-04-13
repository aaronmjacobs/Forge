#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"

#include "Core/Enum.h"

#include "Graphics/SpecializationInfo.h"

// static
void BloomUpsampleShaderConstants::registerMembers(ShaderPermutationManager<BloomUpsampleShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&BloomUpsampleShaderConstants::quality, RenderQuality::Disabled, RenderQuality::High);
   permutationManager.registerMember(&BloomUpsampleShaderConstants::horizontal);
}

// static
std::vector<vk::DescriptorSetLayoutBinding> BloomUpsampleDescriptorSet::getBindings()
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
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

BloomUpsampleShader::BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Screen", "BloomUpsample"))
{
}
