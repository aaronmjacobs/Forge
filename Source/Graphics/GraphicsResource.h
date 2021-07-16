#pragma once

#include "Core/Assert.h"

#include "Graphics/GraphicsContext.h"

#include <string>
#include <string_view>

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

   virtual ~GraphicsResource() = default;

   GraphicsResource& operator=(const GraphicsResource& other) = delete;
   GraphicsResource& operator=(GraphicsResource&& other) = delete;

#if FORGE_DEBUG
   const std::string& getName() const
   {
      return name;
   }

   virtual void setName(std::string_view newName)
   {
      name = newName;
   }
#endif // FORGE_DEBUG

protected:
   const GraphicsContext& context;
   const vk::Device device;

#if FORGE_DEBUG
   std::string name;
#endif // FORGE_DEBUG
};
