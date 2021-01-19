#include "Renderer/View.h"

#include "Core/Math/MathUtils.h"

#include "Graphics/DescriptorSetLayoutCache.h"

#include <glm/gtc/matrix_transform.hpp>

// static
const vk::DescriptorSetLayoutCreateInfo& View::getLayoutCreateInfo()
{
   static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);

   static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &kBinding);

   return kCreateInfo;
}

// static
vk::DescriptorSetLayout View::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

View::View(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
}

void View::update()
{
   glm::mat4 worldToView = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), MathUtils::kUpVector);

   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();
   glm::mat4 viewToClip = glm::perspective(glm::radians(70.0f), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, 0.1f, 10.0f);
   viewToClip[1][1] *= -1.0f; // In Vulkan, the clip space Y coordinates are inverted compared to OpenGL (which GLM was written for), so we flip the sign here

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;

   uniformBuffer.update(viewUniformData);
}

void View::updateDescriptorSets()
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = getDescriptorBufferInfo(frameIndex);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      device.updateDescriptorSets({ viewDescriptorWrite }, {});
   }
}
