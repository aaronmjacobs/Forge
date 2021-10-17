#include "Renderer/Passes/Depth/DepthShader.h"

#include "Renderer/UniformData.h"
#include "Renderer/View.h"

#include "Resources/ResourceManager.h"

DepthShader::DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
{
   ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Depth.vert.spv");
   const ShaderModule* vertShaderModule = resourceManager.getShaderModule(vertModuleHandle);
   if (!vertShaderModule)
   {
      throw std::runtime_error(std::string("Failed to load shader"));
   }

   vertStageCreateInfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vertShaderModule->getShaderModule())
      .setPName("main");
}

void DepthShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, const View& view, vk::PipelineLayout pipelineLayout)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> DepthShader::getStages() const
{
   return { vertStageCreateInfo };
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
