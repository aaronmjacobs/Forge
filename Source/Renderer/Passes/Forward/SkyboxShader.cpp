#include "Renderer/Passes/Forward/SkyboxShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayout.h"

#include "Renderer/View.h"

namespace
{
   Shader::InitializationInfo getInitializationInfo()
   {
      Shader::InitializationInfo info;

      info.vertShaderModulePath = "Resources/Shaders/Screen.vert.spv";
      info.fragShaderModulePath = "Resources/Shaders/Skybox.frag.spv";

      return info;
   }
}

// static
std::array<vk::DescriptorSetLayoutBinding, 1> SkyboxShader::getBindings()
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
const vk::DescriptorSetLayoutCreateInfo& SkyboxShader::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<SkyboxShader>();
}

// static
vk::DescriptorSetLayout SkyboxShader::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<SkyboxShader>(context);
}

SkyboxShader::SkyboxShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : Shader(graphicsContext, resourceManager, getInitializationInfo())
{
}

void SkyboxShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> SkyboxShader::getStages() const
{
   return getStagesForPermutation(0);
}

std::vector<vk::DescriptorSetLayout> SkyboxShader::getSetLayouts() const
{
   return { View::getLayout(context), getLayout(context) };
}
