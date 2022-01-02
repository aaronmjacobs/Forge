#pragma once

#include "Graphics/GraphicsContext.h"

namespace Command
{
   vk::CommandBuffer beginSingle(const GraphicsContext& context);
   void endSingle(const GraphicsContext& context, vk::CommandBuffer commandBuffer);

   template<typename Function>
   void executeSingle(const GraphicsContext& context, Function function)
   {
      vk::CommandBuffer commandBuffer = beginSingle(context);
      function(commandBuffer);
      endSingle(context, commandBuffer);
   }
}
