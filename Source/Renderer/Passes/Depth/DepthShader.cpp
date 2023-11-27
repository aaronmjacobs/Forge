#include "Renderer/Passes/Depth/DepthShader.h"

#include "Renderer/UniformData.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Depth.vert.spv";

      return info;
   }
}

DepthShader::DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ShaderWithDescriptors(graphicsContext, resourceManager, getInitializationInfo())
{
}

std::vector<vk::PipelineShaderStageCreateInfo> DepthShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::PushConstantRange> DepthShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
