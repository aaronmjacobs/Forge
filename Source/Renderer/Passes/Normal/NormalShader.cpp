#include "Renderer/Passes/Normal/NormalShader.h"

#include "Graphics/SpecializationInfo.h"

#include "Renderer/UniformData.h"

// static
void NormalShaderConstants::registerMembers(ShaderPermutationManager<NormalShaderConstants>& permutationManager)
{
   permutationManager.registerMember(&NormalShaderConstants::withTextures);
   permutationManager.registerMember(&NormalShaderConstants::masked);
}

NormalShader::NormalShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : ParameterizedShader(graphicsContext, resourceManager, Shader::ModuleInfo("Normal", "Normal"))
{
}

std::vector<vk::PushConstantRange> NormalShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
