#include "Renderer/Passes/Depth/DepthMaskedShader.h"

#include "Renderer/PhysicallyBasedMaterial.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

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
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void DepthMaskedShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Material& material)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), material.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> DepthMaskedShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::DescriptorSetLayout> DepthMaskedShader::getSetLayouts() const
{
   return { View::getLayout(context), PhysicallyBasedMaterial::getLayout(context) };
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
