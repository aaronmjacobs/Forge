#pragma once

#include "Core/Containers/GenerationalArray.h"
#include "Core/Containers/ReflectedMap.h"

#include "Resources/ResourceTypes.h"

#include <memory>
#include <utility>

template<typename T, typename Identifier>
class ResourceContainer
{
public:
   using Handle = ResourceHandle<T>;

   Handle add(const Identifier& identifier, std::unique_ptr<T> resource)
   {
      Handle handle = resources.emplace(std::move(resource));
      cacheHandle(identifier, handle);

      return handle;
   }

   template<typename... Args>
   Handle emplace(const Identifier& identifier, Args&&... args)
   {
      Handle handle = resources.emplace(std::make_unique<T>(std::forward<Args>(args)...));
      cacheHandle(identifier, handle);

      return handle;
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

   const Identifier* findIdentifier(Handle handle) const
   {
      return cache.find(handle);
   }

   Handle findHandle(const Identifier& identifier) const
   {
      if (const Handle* handle = cache.find(identifier))
      {
         if (resources.get(*handle) != nullptr)
         {
            return *handle;
         }
      }

      return Handle{};
   }

private:
   void cacheHandle(const Identifier& identifier, Handle handle)
   {
      cache.add(identifier, handle);
   }

   GenerationalArray<std::unique_ptr<T>> resources;
   ReflectedMap<Identifier, Handle> cache;
};
