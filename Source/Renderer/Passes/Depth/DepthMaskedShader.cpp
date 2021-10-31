#include "Renderer/Passes/Depth/DepthMaskedShader.h"

#include "Renderer/PhongMaterial.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

DepthMaskedShader::DepthMaskedShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
{
   ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/DepthMasked.vert.spv");
   ShaderModuleHandle fragModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/DepthMasked.frag.spv");

   const ShaderModule* vertShaderModule = resourceManager.getShaderModule(vertModuleHandle);
   const ShaderModule* fragShaderModule = resourceManager.getShaderModule(fragModuleHandle);
   if (!vertShaderModule || !fragShaderModule)
   {
      throw std::runtime_error(std::string("Failed to load shader"));
   }

   vertStageCreateInfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vertShaderModule->getShaderModule())
      .setPName("main");

   fragStageCreateInfo = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule->getShaderModule())
      .setPName("main");
}

void DepthMaskedShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const Material& material)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), material.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> DepthMaskedShader::getStages() const
{
   return { vertStageCreateInfo, fragStageCreateInfo };
}

std::vector<vk::DescriptorSetLayout> DepthMaskedShader::getSetLayouts() const
{
   return { View::getLayout(context), PhongMaterial::getLayout(context) };
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
