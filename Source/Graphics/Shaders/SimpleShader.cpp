#include "Graphics/Shaders/SimpleShader.h"

#include "Graphics/Texture.h"

#include <utility>

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

   SimpleShaderStageData& getStageData()
   {
      static SimpleShaderStageData stageData;
      return stageData;
   }
}

SimpleShader::SimpleShader(ShaderModuleResourceManager& shaderModuleResourceManager, const GraphicsContext& context)
   : GraphicsResource(context)
{
   {
      ShaderModuleHandle vertModuleHandle = shaderModuleResourceManager.load("Resources/Shaders/Simple.vert.spv", context);
      ShaderModuleHandle fragModuleHandle = shaderModuleResourceManager.load("Resources/Shaders/Simple.frag.spv", context);

      const ShaderModule* vertShaderModule = shaderModuleResourceManager.get(vertModuleHandle);
      const ShaderModule* fragShaderModule = shaderModuleResourceManager.get(fragModuleHandle);
      if (!vertShaderModule || !fragShaderModule)
      {
         throw std::runtime_error(std::string("Failed to load shader"));
      }

      vertStageCreateInfo = vk::PipelineShaderStageCreateInfo()
         .setStage(vk::ShaderStageFlagBits::eVertex)
         .setModule(vertShaderModule->getShaderModule())
         .setPName("main");

      SimpleShaderStageData& stageData = getStageData();

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

   {
      vk::DescriptorSetLayoutBinding viewUniformBufferLayoutBinding = vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eVertex);

      vk::DescriptorSetLayoutBinding meshUniformBufferLayoutBinding = vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eVertex);

      vk::DescriptorSetLayoutBinding samplerLayoutBinding = vk::DescriptorSetLayoutBinding()
         .setBinding(1)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment);

      vk::DescriptorSetLayoutCreateInfo frameLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
         .setBindings(viewUniformBufferLayoutBinding);
      frameLayout = device.createDescriptorSetLayout(frameLayoutCreateInfo);

      std::array<vk::DescriptorSetLayoutBinding, 2> drawLayoutBindings =
      {
         meshUniformBufferLayoutBinding,
         samplerLayoutBinding
      };
      vk::DescriptorSetLayoutCreateInfo drawLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
         .setBindings(drawLayoutBindings);
      drawLayout = device.createDescriptorSetLayout(drawLayoutCreateInfo);
   }
}

SimpleShader::~SimpleShader()
{
   ASSERT(device);

   if (frameLayout)
   {
      device.destroyDescriptorSetLayout(frameLayout);
   }

   if (drawLayout)
   {
      device.destroyDescriptorSetLayout(drawLayout);
   }
}

void SimpleShader::allocateDescriptorSets(vk::DescriptorPool descriptorPool, uint32_t numSwapchainImages)
{
   ASSERT(frameLayout && drawLayout);
   ASSERT(!areDescriptorSetsAllocated());

   std::vector<vk::DescriptorSetLayout> frameLayouts(numSwapchainImages, frameLayout);
   std::vector<vk::DescriptorSetLayout> drawLayouts(numSwapchainImages, drawLayout);

   vk::DescriptorSetAllocateInfo frameAllocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(descriptorPool)
      .setSetLayouts(frameLayouts);
   vk::DescriptorSetAllocateInfo drawAllocateInfo = vk::DescriptorSetAllocateInfo()
      .setDescriptorPool(descriptorPool)
      .setSetLayouts(drawLayouts);

   frameSets = device.allocateDescriptorSets(frameAllocateInfo);
   drawSets = device.allocateDescriptorSets(drawAllocateInfo);
}

void SimpleShader::clearDescriptorSets()
{
   frameSets.clear();
   drawSets.clear();
}

void SimpleShader::updateDescriptorSets(const GraphicsContext& context, uint32_t numSwapchainImages, const UniformBuffer<ViewUniformData>& viewUniformBuffer, const UniformBuffer<MeshUniformData>& meshUniformBuffer, const Texture& texture, vk::Sampler sampler)
{
   for (uint32_t i = 0; i < numSwapchainImages; ++i)
   {
      vk::DescriptorBufferInfo viewBufferInfo = viewUniformBuffer.getDescriptorBufferInfo(context, static_cast<uint32_t>(i));
      vk::DescriptorBufferInfo meshBufferInfo = meshUniformBuffer.getDescriptorBufferInfo(context, static_cast<uint32_t>(i));

      vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
         .setImageView(texture.getDefaultView())
         .setSampler(sampler);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(frameSets[i])
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      vk::WriteDescriptorSet meshDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(drawSets[i])
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&meshBufferInfo);

      vk::WriteDescriptorSet imageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(drawSets[i])
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo);

      device.updateDescriptorSets({ viewDescriptorWrite, meshDescriptorWrite, imageDescriptorWrite }, {});
   }
}

void SimpleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t swapchainIndex)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { frameSets[swapchainIndex], drawSets[swapchainIndex] }, {});
}
