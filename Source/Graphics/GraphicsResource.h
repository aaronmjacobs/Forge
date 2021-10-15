#pragma once

#include "Core/Assert.h"

#include "Graphics/GraphicsContext.h"

#if FORGE_DEBUG
#  include <string>
#  include <utility>
#endif // FORGE_DEBUG

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
   GraphicsResource(GraphicsResource&& other)
      : context(other.context)
      , device(other.device)
#if FORGE_DEBUG
      , cachedCompositeName(std::move(other.cachedCompositeName))
#endif // FORGE_DEBUG
   {
      ASSERT(device && device == context.getDevice());

#if FORGE_DEBUG
      onResourceMoved(std::move(other));
#endif // FORGE_DEBUG
   }

   virtual ~GraphicsResource()
   {
#if FORGE_DEBUG
      onResourceDestroyed();
#endif // FORGE_DEBUG
   }

   GraphicsResource& operator=(const GraphicsResource& other) = delete;
   GraphicsResource& operator=(GraphicsResource&& other) = delete;

#if FORGE_DEBUG
   const std::string& getName() const
   {
      return cachedCompositeName;
   }

   void updateCachedCompositeName(std::string name)
   {
      cachedCompositeName = std::move(name);
   }
#endif // FORGE_DEBUG

protected:
   const GraphicsContext& context;
   const vk::Device device;

private:
#if FORGE_DEBUG
   void onResourceMoved(GraphicsResource&& other);
   void onResourceDestroyed();

   std::string cachedCompositeName;
#endif // FORGE_DEBUG
};
