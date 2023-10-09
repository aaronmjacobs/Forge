#pragma once

#include "Core/Containers/GenerationalArray.h"
#include "Core/Containers/ReflectedMap.h"

#include "Resources/ResourceTypes.h"

#include <memory>
#include <utility>

template<typename ResourceKey, typename ResourceValue>
class ResourceContainer
{
public:
   using Handle = ResourceHandle<ResourceValue>;

   Handle add(const ResourceKey& key, std::unique_ptr<ResourceValue> resource)
   {
      Handle handle = resources.emplace(std::move(resource));
      cacheHandle(key, handle);

      return handle;
   }

   template<typename... Args>
   Handle emplace(const ResourceKey& key, Args&&... args)
   {
      Handle handle = resources.emplace(std::make_unique<ResourceValue>(std::forward<Args>(args)...));
      cacheHandle(key, handle);

      return handle;
   }

   bool replace(Handle handle, std::unique_ptr<ResourceValue> resource)
   {
      return resources.replace(handle, std::move(resource));
   }

   template<typename... Args>
   bool replace(Handle handle, Args&&... args)
   {
      return resources.replace(handle, std::make_unique<ResourceValue>(std::forward<Args>(args)...));
   }

   bool remove(Handle handle)
   {
      bool unloaded = resources.remove(handle);

      if (unloaded)
      {
         cache.remove(handle);
      }

      return unloaded;
   }

   void removeAll()
   {
      resources.removeAll();
      cache.clear();
   }

   ResourceValue* get(Handle handle)
   {
      std::unique_ptr<ResourceValue>* resource = resources.get(handle);
      return resource ? resource->get() : nullptr;
   }

   const ResourceValue* get(Handle handle) const
   {
      const std::unique_ptr<ResourceValue>* resource = resources.get(handle);
      return resource ? resource->get() : nullptr;
   }

   const ResourceKey* findKey(Handle handle) const
   {
      return cache.find(handle);
   }

   Handle findHandle(const ResourceKey& key) const
   {
      if (const Handle* handle = cache.find(key))
      {
         if (resources.get(*handle) != nullptr)
         {
            return *handle;
         }
      }

      return Handle{};
   }

private:
   void cacheHandle(const ResourceKey& key, Handle handle)
   {
      cache.add(key, handle);
   }

   GenerationalArray<std::unique_ptr<ResourceValue>> resources;
   ReflectedMap<ResourceKey, Handle> cache;
};
