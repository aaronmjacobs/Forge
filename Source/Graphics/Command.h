#pragma once

#include "Graphics/GraphicsContext.h"

namespace Command
{
   vk::CommandBuffer beginSingle(const GraphicsContext& context);
   void endSingle(const GraphicsContext& context, vk::CommandBuffer commandBuffer);
}
