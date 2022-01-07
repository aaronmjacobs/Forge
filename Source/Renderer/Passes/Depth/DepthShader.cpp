#include "Renderer/Passes/Depth/DepthShader.h"

#include "Renderer/UniformData.h"
#include "Renderer/View.h"

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
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void DepthShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> DepthShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::DescriptorSetLayout> DepthShader::getSetLayouts() const
{
   return { View::getLayout(context) };
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
