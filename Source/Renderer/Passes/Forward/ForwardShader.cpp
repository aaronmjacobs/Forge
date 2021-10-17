#include "Renderer/Passes/Forward/ForwardShader.h"

#include "Graphics/DescriptorSet.h"
#include "Graphics/Texture.h"

#include "Renderer/ForwardLighting.h"
#include "Renderer/PhongMaterial.h"
#include "Renderer/UniformData.h"
#include "Renderer/View.h"

#include "Resources/ResourceManager.h"

namespace
{
   struct ForwardShaderStageData
   {
      std::array<vk::SpecializationMapEntry, 1> specializationMapEntries =
      {
         vk::SpecializationMapEntry()
            .setConstantID(0)
            .setOffset(0)
            .setSize(sizeof(VkBool32))
      };

      std::array<VkBool32, 1> withTexturesSpecializationData = { true };
      std::array<VkBool32, 1> withoutTexturesSpecializationData = { false };

      vk::SpecializationInfo withTextureSpecializationInfo = vk::SpecializationInfo()
         .setMapEntries(specializationMapEntries)
         .setData<VkBool32>(withTexturesSpecializationData);
      vk::SpecializationInfo withoutTexturesSpecializationInfo = vk::SpecializationInfo()
         .setMapEntries(specializationMapEntries)
         .setData<VkBool32>(withoutTexturesSpecializationData);
   };

   const ForwardShaderStageData& getStageData()
   {
      static const ForwardShaderStageData kStageData;
      return kStageData;
   }
}

ForwardShader::ForwardShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
{
   ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Forward.vert.spv");
   ShaderModuleHandle fragModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Forward.frag.spv");

   const ShaderModule* vertShaderModule = resourceManager.getShaderModule(vertModuleHandle);
   const ShaderModule* fragShaderModule = resourceManager.getShaderModule(fragModuleHandle);
   if (!vertShaderModule || !fragShaderModule)
   {
      throw std::runtime_error(std::string("Failed to load shader"));
   }

   const ForwardShaderStageData& stageData = getStageData();

   vertStageCreateInfoWithTextures = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vertShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&stageData.withTextureSpecializationInfo);

   vertStageCreateInfoWithoutTextures = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vertShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&stageData.withoutTexturesSpecializationInfo);

   fragStageCreateInfoWithTextures = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&stageData.withTextureSpecializationInfo);

   fragStageCreateInfoWithoutTextures = vk::PipelineShaderStageCreateInfo()
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(fragShaderModule->getShaderModule())
      .setPName("main")
      .setPSpecializationInfo(&stageData.withoutTexturesSpecializationInfo);
}

void ForwardShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const View& view, const ForwardLighting& lighting, const Material& material)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { view.getDescriptorSet().getCurrentSet(), lighting.getDescriptorSet().getCurrentSet(), material.getDescriptorSet().getCurrentSet() }, {});
}

std::vector<vk::PipelineShaderStageCreateInfo> ForwardShader::getStages(bool withTextures) const
{
   return { withTextures ? vertStageCreateInfoWithTextures : vertStageCreateInfoWithoutTextures, withTextures ? fragStageCreateInfoWithTextures : fragStageCreateInfoWithoutTextures };
}

std::vector<vk::DescriptorSetLayout> ForwardShader::getSetLayouts() const
{
   return { View::getLayout(context), ForwardLighting::getLayout(context), PhongMaterial::getLayout(context) };
}

std::vector<vk::PushConstantRange> ForwardShader::getPushConstantRanges() const
{
   return
   {
      vk::PushConstantRange()
         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
         .setOffset(0)
         .setSize(sizeof(MeshUniformData))
   };
}
