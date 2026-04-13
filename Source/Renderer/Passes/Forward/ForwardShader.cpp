#include "Renderer/Passes/Forward/ForwardShader.h"

#include "Graphics/SpecializationInfo.h"

#include "Renderer/UniformData.h"

// static
void ForwardShaderConstants::registerMembers(ShaderPermutationManager<ForwardShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&ForwardShaderConstants::withTextures);
   permutationManager.registerMember(&ForwardShaderConstants::withBlending);
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
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Forward", "Forward"))
{
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
