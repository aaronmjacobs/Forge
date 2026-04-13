#include "Renderer/Passes/Forward/SkyboxShader.h"

// static
std::vector<vk::DescriptorSetLayoutBinding> SkyboxDescriptorSet::getBindings()
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

SkyboxShader::SkyboxShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Screen", "Skybox"))
{
}
