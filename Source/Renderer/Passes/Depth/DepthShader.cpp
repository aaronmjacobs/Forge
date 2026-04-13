#include "Renderer/Passes/Depth/DepthShader.h"

#include "Renderer/UniformData.h"

DepthShader::DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Depth", ""))
{
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
