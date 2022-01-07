#include "Renderer/Passes/PostProcess/Tonemap/TonemapShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Tonemap.frag.spv";

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> TonemapShader::getBindings()
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

// static
const vk::DescriptorSetLayoutCreateInfo& TonemapShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<TonemapShader>();
}

// static
vk::DescriptorSetLayout TonemapShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<TonemapShader>(context);
}

TonemapShader::TonemapShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void TonemapShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> TonemapShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::DescriptorSetLayout> TonemapShader::getSetLayouts() const
{
   return { getLayout(context) };
}
