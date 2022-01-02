#include "Renderer/Passes/Composite/CompositeShader.h"

#include "Core/Enum.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/DescriptorSetLayoutCache.h"

#include "Resources/ResourceManager.h"

namespace
{
   struct CompositeShaderStageData
   {
      vk::SpecializationMapEntry specializationMapEntry = vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0)
            .setSize(sizeof(int));

      std::array<std::array<int, 1>, CompositeShader::kNumModes> specializationData =
      { {
         { 0 },
         { 1 },
         { 2 }
      } };

      std::array<vk::SpecializationInfo, CompositeShader::kNumModes> specializationInfo =
      {
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<int>(specializationData[0]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<int>(specializationData[1]),
         vk::SpecializationInfo().setMapEntries(specializationMapEntry).setData<int>(specializationData[2])
      };
   };

   const CompositeShaderStageData& getStageData()
   {
      static const CompositeShaderStageData kStageData;
      return kStageData;
   }
}

// static
const vk::DescriptorSetLayoutCreateInfo& CompositeShader::getLayoutCreateInfo()
{
   static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eFragment);

   static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &kBinding);

   return kCreateInfo;
}

// static
vk::DescriptorSetLayout CompositeShader::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

CompositeShader::CompositeShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
{
   ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Screen.vert.spv");
   ShaderModuleHandle fragModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Composite.frag.spv");

   const ShaderModule* vertShaderModule = resourceManager.getShaderModule(vertModuleHandle);
   const ShaderModule* fragShaderModule = resourceManager.getShaderModule(fragModuleHandle);
   if (!vertShaderModule || !fragShaderModule)
   {
      throw std::runtime_error(std::string("Failed to load shader"));
   }

   const CompositeShaderStageData& stageData = getStageData();

   for (uint32_t i = 0; i < kNumModes; ++i)
   {
      vertStageCreateInfo[i] = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eVertex)
         .setModule(vertShaderModule->getShaderModule())
         .setPName("main")
         .setPSpecializationInfo(&stageData.specializationInfo[i]);

      fragStageCreateInfo[i] = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eFragment)
         .setModule(fragShaderModule->getShaderModule())
         .setPName("main")
         .setPSpecializationInfo(&stageData.specializationInfo[i]);
   }
}

void CompositeShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const DescriptorSet& descriptorSet)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { descriptorSet.getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> CompositeShader::getStages(Mode mode) const
{
   return { vertStageCreateInfo[Enum::cast(mode)], fragStageCreateInfo[Enum::cast(mode)]};
}

std::vector<vk::DescriptorSetLayout> CompositeShader::getSetLayouts() const
{
   return { getLayout(context) };
}
