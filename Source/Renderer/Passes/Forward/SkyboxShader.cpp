#include "Renderer/Passes/Forward/SkyboxShader.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Skybox.frag.spv";

      return info;
   }
}

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
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> SkyboxShader::getStages() const
{
   return getStagesForPermutation(0);
}
