#include "Renderer/Passes/Composite/CompositeShader.h"

#include "Core/Enum.h"

#include "Graphics/ShaderPermutationManager.h"

// static
void CompositeShaderConstants::registerMembers(ShaderPermutationManager<CompositeShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&CompositeShaderConstants::mode, CompositeMode::Passthrough, CompositeMode::SrgbToLinear);
}

// static
std::vector<vk::DescriptorSetLayoutBinding> CompositeDescriptorSet::getBindings()
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

CompositeShader::CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Screen", "Composite"))
{
}
