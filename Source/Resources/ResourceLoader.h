#pragma once

#include "Resources/ResourceContainer.h"
#include "Resources/ResourceTypes.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

class GraphicsContext;
class ResourceManager;

namespace ResourceLoadHelpers
{
   std::optional<std::filesystem::path> makeCanonical(const std::filesystem::path& path);

#if FORGE_WITH_DEBUG_UTILS
   std::string getName(const std::filesystem::path& path);
#endif // FORGE_WITH_DEBUG_UTILS
}

template<typename T, typename Identifier>
class ResourceLoader
{
public:
   using Handle = ResourceHandle<T>;

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
      onAllResourcesUnloaded();
   }

   void clear()
   {
      container.clear();
      onAllResourcesUnloaded();
   }

   T* get(Handle handle)
   {
      return container.get(handle);
   }

   const T* get(Handle handle) const
   {
      return container.get(handle);
   }

   const Identifier* findIdentifier(Handle handle) const
   {
      return container.findIdentifier(handle);
   }

protected:
   virtual void onAllResourcesUnloaded()
   {
   }

   const GraphicsContext& context;
   ResourceManager& resourceManager;
   ResourceContainer<T, Identifier> container;
};
