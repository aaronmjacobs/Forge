#pragma once

#include "Core/Assert.h"

#include "Graphics/GraphicsContext.h"

class GraphicsResource
{
public:
   GraphicsResource(const GraphicsContext& graphicsContext)
      : context(graphicsContext)
      , device(context.getDevice())
   {
      ASSERT(device);
   }

   GraphicsResource(const GraphicsResource& other) = delete;
   GraphicsResource(GraphicsResource&& other) = delete;

   ~GraphicsResource() = default;

   GraphicsResource& operator=(const GraphicsResource& other) = delete;
   GraphicsResource& operator=(GraphicsResource&& other) = delete;

protected:
   const GraphicsContext& context;
   const vk::Device device;
};
