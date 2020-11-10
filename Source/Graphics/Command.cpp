#include "Graphics/Command.h"

namespace Command
{
   vk::CommandBuffer beginSingle(const VulkanContext& context)
   {
      vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
         .setLevel(vk::CommandBufferLevel::ePrimary)
         .setCommandPool(context.transientCommandPool)
         .setCommandBufferCount(1);

      std::vector<vk::CommandBuffer> commandBuffers = context.device.allocateCommandBuffers(commandBufferAllocateInfo);
      ASSERT(commandBuffers.size() == 1);
      vk::CommandBuffer commandBuffer = commandBuffers[0];

      vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      commandBuffer.begin(beginInfo);

      return commandBuffer;
   }

   void endSingle(const VulkanContext& context, vk::CommandBuffer commandBuffer)
   {
      commandBuffer.end();

      vk::SubmitInfo submitInfo = vk::SubmitInfo()
         .setCommandBufferCount(1)
         .setPCommandBuffers(&commandBuffer);

      context.graphicsQueue.submit({ submitInfo }, nullptr);
      context.graphicsQueue.waitIdle();

      context.device.freeCommandBuffers(context.transientCommandPool, commandBuffer);
   }
}
