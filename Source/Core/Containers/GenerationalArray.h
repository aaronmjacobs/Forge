#pragma once

#include "Core/Assert.h"
#include "Core/Log.h"

#include <cstdint>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

template<typename T>
class GenerationalArrayHandle
{
public:
   GenerationalArrayHandle()
      : index(0)
      , version(0)
   {
   }
   
   bool isValid() const
   {
      return version > 0;
   }
   
   void reset()
   {
      index = 0;
      version = 0;
   }
   
   explicit operator bool() const
   {
      return isValid();
   }
   
   bool operator==(const GenerationalArrayHandle& other) const
   {
      return index == other.index && version == other.version;
   }
   
   std::size_t hash() const
   {
      return std::hash<uint32_t>{}((version << 24) | index);
   }
   
private:
   template<typename U>
   friend class GenerationalArray;
   
   GenerationalArrayHandle(uint32_t indexValue, uint8_t versionValue)
      : index(indexValue)
      , version(versionValue)
   {
      static_assert(sizeof(GenerationalArrayHandle) == 4);
      ASSERT(indexValue < (1 << 24));
   }
   
   uint32_t index : 24;
   uint32_t version : 8;
};

namespace std
{
   template<typename T>
   struct hash<GenerationalArrayHandle<T>>
   {
      std::size_t operator()(const GenerationalArrayHandle<T>& handle) const
      {
         return handle.hash();
      }
   };
}

template<typename T>
class GenerationalArray
{
public:
   using Handle = GenerationalArrayHandle<T>;
   
   Handle add(const T& value)
   {
      std::size_t index = allocate();
      
      elements[index] = value;

      return createHandle(index);
   }
   
   Handle add(T&& value)
   {
      std::size_t index = allocate();
      
      elements[index] = std::move(value);

      return createHandle(index);
   }
   
   template<typename... Args>
   Handle emplace(Args&&... args)
   {
      std::size_t index = allocate();
      
      elements[index].emplace(std::forward<Args>(args)...);

      return createHandle(index);
   }
   
   bool remove(Handle handle)
   {
      if (std::optional<T>* element = find(handle))
      {
         element->reset();
         freeIndices.push(handle.index);
         return true;
      }
      
      return false;
   }
   
   void removeAll()
   {
      for (std::size_t i = 0; i < elements.size(); ++i)
      {
         if (elements[i])
         {
            elements[i].reset();
            freeIndices.push(i);
         }
      }
   }
   
   void clear()
   {
      elements.clear();
      versions.clear();
      freeIndices = std::queue<std::size_t>();
   }
   
   T* get(Handle handle)
   {
      if (std::optional<T>* element = find(handle))
      {
         ASSERT(element->has_value());
         return &element->value();
      }
      
      return nullptr;
   }
   
   const T* get(Handle handle) const
   {
      if (const std::optional<T>* element = find(handle))
      {
         ASSERT(element->has_value());
         return &element->value();
      }
      
      return nullptr;
   }
   
private:
   std::size_t allocate()
   {
      std::size_t index = 0;

      if (freeIndices.empty())
      {
         index = elements.size();

         elements.emplace_back();
         versions.emplace_back(0);
      }
      else
      {
         index = freeIndices.front();

         freeIndices.pop();
      }
      
      return index;
   }
   
   Handle createHandle(std::size_t index)
   {
      ASSERT(elements.size() == versions.size());
      ASSERT(index < versions.size());
      ASSERT(index < (1 << 24));
      
#if FORGE_DEBUG
      if (versions[index] == 255)
      {
         LOG_WARNING("Generational array version overflow (index " << index << ")");
      }
#endif // FORGE_DEBUG
      
      uint8_t version = ++versions[index];
      return Handle(static_cast<uint32_t>(index), version);
   }
   
   std::optional<T>* find(Handle handle)
   {
      ASSERT(elements.size() == versions.size());

      if (handle.index < elements.size() && handle.version == versions[handle.index])
      {
         std::optional<T>& element = elements[handle.index];
         if (element)
         {
            return &element;
         }
      }
      
      return nullptr;
   }
   
   const std::optional<T>* find(Handle handle) const
   {
      ASSERT(elements.size() == versions.size());

      if (handle.index < elements.size() && handle.version == versions[handle.index])
      {
         const std::optional<T>& element = elements[handle.index];
         if (element)
         {
            return &element;
         }
      }
      
      return nullptr;
   }
   
   std::vector<std::optional<T>> elements;
   std::vector<uint8_t> versions;
   std::queue<std::size_t> freeIndices;
};
