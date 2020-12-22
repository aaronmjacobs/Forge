#pragma once

#include "Core/Containers/GenerationalArray.h"
#include "Core/Containers/ReflectedMap.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

namespace ResourceHelpers
{
   std::optional<std::filesystem::path> makeCanonical(const std::filesystem::path& path);
}

template<typename T>
class ResourceManagerBase
{
public:
   using Handle = typename GenerationalArray<std::unique_ptr<T>>::Handle;

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
   }

   void clear()
   {
      resources.clear();
      cache.clear();
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

   void cacheHandle(Handle handle, const std::string& canonicalPathString)
   {
      cache.add(handle, canonicalPathString);
   }

   std::optional<Handle> getCachedHandle(const std::string& canonicalPathString) const
   {
      if (const Handle* handle = cache.find(canonicalPathString))
      {
         if (resources.get(*handle) != nullptr)
         {
            return *handle;
         }
      }

      return {};
   }

private:
   GenerationalArray<std::unique_ptr<T>> resources;
   ReflectedMap<Handle, std::string> cache;
};
