#include "Renderer/Passes/Depth/DepthMaskedShader.h"

#include "Renderer/UniformData.h"

DepthMaskedShader::DepthMaskedShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("DepthMasked", "DepthMasked"))
{
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
