#include "Renderer/Passes/PostProcess/Bloom/BloomUpsampleShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/BloomUpsample.frag.spv";

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 3> BloomUpsampleShader::getBindings()
{
   return
{
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(1)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(2)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
   };
}

// static
const vk::DescriptorSetLayoutCreateInfo& BloomUpsampleShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<BloomUpsampleShader>();
}

// static
vk::DescriptorSetLayout BloomUpsampleShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<BloomUpsampleShader>(context);
}

BloomUpsampleShader::BloomUpsampleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void BloomUpsampleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> BloomUpsampleShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::DescriptorSetLayout> BloomUpsampleShader::getSetLayouts() const
{
   return { getLayout(context) };
}
