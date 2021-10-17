#include "Graphics/GraphicsResource.h"

#include "Graphics/DebugUtils.h"

#if FORGE_DEBUG
void GraphicsResource::onResourceMoved(GraphicsResource&& other)
{
   DebugUtils::onResourceMoved(&other, this);
}

void GraphicsResource::onResourceDestroyed()
{
   DebugUtils::onResourceDestroyed(this);
}
#endif // FORGE_DEBUG
