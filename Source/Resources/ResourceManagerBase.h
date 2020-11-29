#pragma once

#include "Core/Containers/GenerationalArray.h"
#include "Core/Containers/ReflectedMap.h"

#include <filesystem>
#include <optional>

namespace ResourceHelpers
{
   std::optional<std::filesystem::path> makeCanonical(const std::filesystem::path& path);
}

template<typename T>
class ResourceManagerBase
{
public:
   using Handle = typename GenerationalArray<T>::Handle;
   
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
      return resources.get(handle);
   }
   
   const T* get(Handle handle) const
   {
      return resources.get(handle);
   }
   
protected:
   template<typename... Args>
   Handle emplaceResource(Args&&... args)
   {
      return resources.emplace(std::forward<Args>(args)...);
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
   GenerationalArray<T> resources;
   ReflectedMap<Handle, std::string> cache;
};
