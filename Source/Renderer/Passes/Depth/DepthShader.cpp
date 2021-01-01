#include "Renderer/Passes/Depth/DepthShader.h"

#include "Graphics/Texture.h"

#include "Renderer/UniformData.h"
#include "Renderer/View.h"

#include <utility>

DepthShader::DepthShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
{
   {
      ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Depth.vert.spv", context);

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
}

DepthShader::~DepthShader()
{
   ASSERT(device);
}

void DepthShader::updateDescriptorSets(const View& view)
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = view.getDescriptorBufferInfo(frameIndex);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(view.getDescriptorSet().getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      device.updateDescriptorSets({ viewDescriptorWrite }, {});
   }
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
