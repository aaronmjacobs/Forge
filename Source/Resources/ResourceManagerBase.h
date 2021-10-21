#pragma once

#include "Core/Containers/GenerationalArray.h"
#include "Core/Containers/ReflectedMap.h"

#include "Resources/ResourceTypes.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

class GraphicsContext;
class ResourceManager;

namespace ResourceHelpers
{
   std::optional<std::filesystem::path> makeCanonical(const std::filesystem::path& path);

#if FORGE_DEBUG
   std::string getName(const std::filesystem::path& path);
#endif // FORGE_DEBUG
}

template<typename T, typename Identifier>
class ResourceManagerBase
{
public:
   using Handle = ResourceHandle<T>;

   ResourceManagerBase(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
      : context(graphicsContext)
      , resourceManager(owningResourceManager)
   {
   }

   bool unload(Handle handle)
   {
      bool unloaded = resources.remove(handle);

      if (unloaded)
      {
         cache.remove(handle);
      }

      return unloaded;
   }

   void unloadAll()
   {
      resources.removeAll();
      cache.clear();
      onAllResourcesUnloaded();
   }

   void clear()
   {
      resources.clear();
      cache.clear();
      onAllResourcesUnloaded();
   }

   T* get(Handle handle)
   {
      std::unique_ptr<T>* resource = resources.get(handle);
      return resource ? resource->get() : nullptr;
   }

   const T* get(Handle handle) const
   {
      const std::unique_ptr<T>* resource = resources.get(handle);
      return resource ? resource->get() : nullptr;
   }

protected:
   Handle addResource(std::unique_ptr<T> resource)
   {
      return resources.emplace(std::move(resource));
   }

   template<typename... Args>
   Handle emplaceResource(Args&&... args)
   {
      return resources.emplace(std::make_unique<T>(std::forward<Args>(args)...));
   }

   void cacheHandle(const Identifier& identifier, Handle handle)
   {
      cache.add(identifier, handle);
   }

   std::optional<Handle> getCachedHandle(const Identifier& identifier) const
   {
      if (const Handle* handle = cache.find(identifier))
      {
         if (resources.get(*handle) != nullptr)
         {
            return *handle;
         }
      }

      return {};
   }

   virtual void onAllResourcesUnloaded()
   {
   }

   const GraphicsContext& context;
   ResourceManager& resourceManager;

private:
   GenerationalArray<std::unique_ptr<T>> resources;
   ReflectedMap<Identifier, Handle> cache;
};
