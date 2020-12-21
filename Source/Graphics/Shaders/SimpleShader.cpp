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

SimpleShader::SimpleShader(ShaderModuleResourceManager& shaderModuleResourceManager, const VulkanContext& context)
   : device(context.device)
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

SimpleShader::SimpleShader(SimpleShader&& other)
{
   move(std::move(other));
}

SimpleShader::~SimpleShader()
{
   release();
}

SimpleShader& SimpleShader::operator=(SimpleShader&& other)
{
   move(std::move(other));
   release();

   return *this;
}

void SimpleShader::move(SimpleShader&& other)
{
   ASSERT(!device);
   device = other.device;
   other.device = nullptr;

   vertStageCreateInfo = other.vertStageCreateInfo;
   fragStageCreateInfoWithTexture = other.fragStageCreateInfoWithTexture;
   fragStageCreateInfoWithoutTexture = other.fragStageCreateInfoWithoutTexture;

   ASSERT(stages.empty());
   stages = std::move(other.stages);

   ASSERT(!frameLayout);
   frameLayout = other.frameLayout;
   other.frameLayout = nullptr;

   ASSERT(!drawLayout);
   drawLayout = other.drawLayout;
   other.drawLayout = nullptr;

   ASSERT(frameSets.empty());
   frameSets = std::move(other.frameSets);

   ASSERT(drawSets.empty());
   drawSets = std::move(other.drawSets);
}

void SimpleShader::release()
{
   if (device)
   {
      if (frameLayout)
      {
         device.destroyDescriptorSetLayout(frameLayout);
         frameLayout = nullptr;
      }

      if (drawLayout)
      {
         device.destroyDescriptorSetLayout(drawLayout);
         drawLayout = nullptr;
      }
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

void SimpleShader::updateDescriptorSets(const VulkanContext& context, uint32_t numSwapchainImages, const UniformBuffer<ViewUniformData>& viewUniformBuffer, const UniformBuffer<MeshUniformData>& meshUniformBuffer, const Texture& texture, vk::Sampler sampler)
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

      context.device.updateDescriptorSets({ viewDescriptorWrite, meshDescriptorWrite, imageDescriptorWrite }, {});
   }
}

void SimpleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t swapchainIndex)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { frameSets[swapchainIndex], drawSets[swapchainIndex] }, {});
}
