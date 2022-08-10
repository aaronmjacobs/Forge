#include "Renderer/Passes/PostProcess/Bloom/BloomDownsampleShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/BloomDownsample.frag.spv";

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> BloomDownsampleShader::getBindings()
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
const vk::DescriptorSetLayoutCreateInfo& BloomDownsampleShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<BloomDownsampleShader>();
}

// static
vk::DescriptorSetLayout BloomDownsampleShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<BloomDownsampleShader>(context);
}

BloomDownsampleShader::BloomDownsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void BloomDownsampleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomDownsampleShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::DescriptorSetLayout> BloomDownsampleShader::getSetLayouts() const
{
   return { getLayout(context) };
}
