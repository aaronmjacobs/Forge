#include "Renderer/Passes/Depth/DepthMaskedShader.h"

#include "Renderer/UniformData.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/DepthMasked.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/DepthMasked.frag.spv";

      return info;
   }
}

DepthMaskedShader::DepthMaskedShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> DepthMaskedShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::PushConstantRange> DepthMaskedShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
