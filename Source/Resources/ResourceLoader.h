#pragma once

#include "Graphics/GraphicsTypes.h"

#include "Resources/ResourceContainer.h"
#include "Resources/ResourceTypes.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

class ResourceManager;

namespace ResourceLoadHelpers
{
   std::optional<std::filesystem::path> makeCanonical(const std::filesystem::path& path);

#if FORGE_WITH_DEBUG_UTILS
   std::string getName(const std::filesystem::path& path);
#endif // FORGE_WITH_DEBUG_UTILS
}

template<typename ResourceKey, typename ResourceValue>
class ResourceLoader
{
public:
   using Handle = ResourceHandle<ResourceValue>;

   ResourceLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
      : context(graphicsContext)
      , resourceManager(owningResourceManager)
   {
   }

   bool unload(Handle handle)
   {
      return container.remove(handle);
   }

   void unloadAll()
   {
      container.removeAll();
   }

   ResourceValue* get(Handle handle)
   {
      return container.get(handle);
   }

   const ResourceValue* get(Handle handle) const
   {
      return container.get(handle);
   }

   const ResourceKey* findKey(Handle handle) const
   {
      return container.findKey(handle);
   }

   ResourceManager& getResourceManager()
   {
      return resourceManager;
   }

protected:
   const GraphicsContext& context;
   ResourceManager& resourceManager;
   ResourceContainer<ResourceKey, ResourceValue> container;
};
