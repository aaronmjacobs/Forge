#include "Renderer/Passes/Simple/SimpleShader.h"

#include "Graphics/Texture.h"

#include "Renderer/UniformData.h"
#include "Renderer/View.h"

namespace
{
   struct SimpleShaderStageData
   {
      std::array<vk::SpecializationMapEntry, 1> specializationMapEntries =
      {
         vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0)
            .setSize(sizeof(VkBool32))
      };

      std::array<VkBool32, 1> withTextureSpecializationData = { true };
      std::array<VkBool32, 1> withoutTextureSpecializationData = { false };

      vk::SpecializationInfo withTextureSpecializationInfo = vk::SpecializationInfo()
         .setMapEntries(specializationMapEntries)
         .setData<VkBool32>(withTextureSpecializationData);
      vk::SpecializationInfo withoutTextureSpecializationInfo = vk::SpecializationInfo()
         .setMapEntries(specializationMapEntries)
         .setData<VkBool32>(withoutTextureSpecializationData);
   };

   const SimpleShaderStageData& getStageData()
   {
      static const SimpleShaderStageData kStageData;
      return kStageData;
   }

   const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo()
   {
      static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment);

      static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &kBinding);

      return kCreateInfo;
   }
}

SimpleShader::SimpleShader(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
   ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Simple.vert.spv");
   ShaderModuleHandle fragModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Simple.frag.spv");

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

   const SimpleShaderStageData& stageData = getStageData();

   fragStageCreateInfoWithTexture = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&stageData.withTextureSpecializationInfo);
   fragStageCreateInfoWithoutTexture = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&stageData.withoutTextureSpecializationInfo);
}

void SimpleShader::updateDescriptorSets(const View& view, const Texture& texture, vk::Sampler sampler)
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = view.getDescriptorBufferInfo(frameIndex);

      vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(texture.getDefaultView())
         .setSampler(sampler);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(view.getDescriptorSet().getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      vk::WriteDescriptorSet imageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo);

      device.updateDescriptorSets({ viewDescriptorWrite, imageDescriptorWrite }, {});
   }
}

void SimpleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, const View& view, vk::PipelineLayout pipelineLayout)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> SimpleShader::getStages(bool withTexture) const
{
   return { vertStageCreateInfo, withTexture ? fragStageCreateInfoWithTexture : fragStageCreateInfoWithoutTexture };
}

std::vector<vk::DescriptorSetLayout> SimpleShader::getSetLayouts() const
{
   return { View::getLayout(context), descriptorSet.getLayout() };
}

std::vector<vk::PushConstantRange> SimpleShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
