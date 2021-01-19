#include "Renderer/Passes/Simple/SimpleShader.h"

#include "Graphics/Texture.h"

#include "Renderer/SimpleMaterial.h"
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
}

SimpleShader::SimpleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
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

void SimpleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, const View& view, vk::PipelineLayout pipelineLayout, const Material& material)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), material.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> SimpleShader::getStages(bool withTexture) const
{
   return { vertStageCreateInfo, withTexture ? fragStageCreateInfoWithTexture : fragStageCreateInfoWithoutTexture };
}

std::vector<vk::DescriptorSetLayout> SimpleShader::getSetLayouts() const
{
   return { View::getLayout(context), SimpleMaterial::getLayout(context) };
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
