#include "Renderer/Passes/Simple/SimpleShader.h"

#include "Graphics/Swapchain.h"
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

SimpleShader::SimpleShader(const GraphicsContext& graphicsContext, ResourceManager& resourceManager)
   : GraphicsResource(graphicsContext)
{
   {
      ShaderModuleHandle vertModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Simple.vert.spv", context);
      ShaderModuleHandle fragModuleHandle = resourceManager.loadShaderModule("Resources/Shaders/Simple.frag.spv", context);

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

      vk::DescriptorSetLayoutBinding samplerLayoutBinding = vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment);

      vk::DescriptorSetLayoutCreateInfo frameLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
         .setBindings(viewUniformBufferLayoutBinding);
      frameLayout = device.createDescriptorSetLayout(frameLayoutCreateInfo);

      std::array<vk::DescriptorSetLayoutBinding, 1> drawLayoutBindings =
      {
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

void SimpleShader::allocateDescriptorSets(vk::DescriptorPool descriptorPool)
{
   ASSERT(frameLayout && drawLayout);
   ASSERT(!areDescriptorSetsAllocated());

   uint32_t swapchainImageCount = context.getSwapchain().getImageCount();
   std::vector<vk::DescriptorSetLayout> frameLayouts(swapchainImageCount, frameLayout);
   std::vector<vk::DescriptorSetLayout> drawLayouts(swapchainImageCount, drawLayout);

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

void SimpleShader::updateDescriptorSets(const UniformBuffer<ViewUniformData>& viewUniformBuffer, const Texture& texture, vk::Sampler sampler)
{
   for (uint32_t i = 0; i < context.getSwapchain().getImageCount(); ++i)
   {
      vk::DescriptorBufferInfo viewBufferInfo = viewUniformBuffer.getDescriptorBufferInfo(i);

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

      vk::WriteDescriptorSet imageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(drawSets[i])
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo);

      device.updateDescriptorSets({ viewDescriptorWrite, imageDescriptorWrite }, {});
   }
}

void SimpleShader::bindDescriptorSets(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout)
{
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { frameSets[context.getSwapchainIndex()], drawSets[context.getSwapchainIndex()] }, {});
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
