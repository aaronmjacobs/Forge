#pragma once

#include "Graphics/Context.h"

namespace Command
{
   vk::CommandBuffer beginSingle(const VulkanContext& context);
   void endSingle(const VulkanContext& context, vk::CommandBuffer commandBuffer);
}
