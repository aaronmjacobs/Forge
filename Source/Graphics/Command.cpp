#include "Graphics/Command.h"

namespace Command
{
   vk::CommandBuffer beginSingle(const GraphicsContext& context)
   {
      vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
         .setLevel(vk::CommandBufferLevel::ePrimary)
         .setCommandPool(context.getTransientCommandPool())
         .setCommandBufferCount(1);

      std::vector<vk::CommandBuffer> commandBuffers = context.getDevice().allocateCommandBuffers(commandBufferAllocateInfo);
      ASSERT(commandBuffers.size() == 1);
      vk::CommandBuffer commandBuffer = commandBuffers[0];

      vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
         .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      commandBuffer.begin(beginInfo);

      return commandBuffer;
   }

   void endSingle(const GraphicsContext& context, vk::CommandBuffer commandBuffer)
   {
      commandBuffer.end();

      vk::SubmitInfo submitInfo = vk::SubmitInfo()
         .setCommandBufferCount(1)
         .setPCommandBuffers(&commandBuffer);

      context.getGraphicsQueue().submit({ submitInfo }, nullptr);
      context.getGraphicsQueue().waitIdle();

      context.getDevice().freeCommandBuffers(context.getTransientCommandPool(), commandBuffer);
   }
}
