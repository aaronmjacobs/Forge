#pragma once

#include "Core/Assert.h"

#include "Graphics/Context.h"

class GraphicsResource
{
public:
   GraphicsResource(const VulkanContext& context)
      : device(context.device)
   {
      ASSERT(device);
   }

   GraphicsResource(const GraphicsResource& other) = delete;
   GraphicsResource(GraphicsResource&& other) = delete;

   ~GraphicsResource() = default;

   GraphicsResource& operator=(const GraphicsResource& other) = delete;
   GraphicsResource& operator=(GraphicsResource&& other) = delete;

protected:
   const vk::Device device;
};
