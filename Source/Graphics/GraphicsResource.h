#pragma once

#include "Core/Assert.h"

#include "Graphics/GraphicsContext.h"

#if FORGE_WITH_DEBUG_UTILS
#  include <string>
#  include <utility>
#endif // FORGE_WITH_DEBUG_UTILS

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
#if FORGE_WITH_DEBUG_UTILS
      , cachedCompositeName(std::move(other.cachedCompositeName))
#endif // FORGE_WITH_DEBUG_UTILS
   {
      ASSERT(device && device == context.getDevice());

#if FORGE_WITH_DEBUG_UTILS
      onResourceMoved(std::move(other));
#endif // FORGE_WITH_DEBUG_UTILS
   }

   virtual ~GraphicsResource()
   {
#if FORGE_WITH_DEBUG_UTILS
      onResourceDestroyed();
#endif // FORGE_WITH_DEBUG_UTILS
   }

   GraphicsResource& operator=(const GraphicsResource& other) = delete;
   GraphicsResource& operator=(GraphicsResource&& other) = delete;

#if FORGE_WITH_DEBUG_UTILS
   const std::string& getName() const
   {
      return cachedCompositeName;
   }

   void updateCachedCompositeName(std::string name)
   {
      cachedCompositeName = std::move(name);
   }
#endif // FORGE_WITH_DEBUG_UTILS

protected:
   const GraphicsContext& context;
   const vk::Device device;

private:
#if FORGE_WITH_DEBUG_UTILS
   void onResourceMoved(GraphicsResource&& other);
   void onResourceDestroyed();

   std::string cachedCompositeName;
#endif // FORGE_WITH_DEBUG_UTILS
};
