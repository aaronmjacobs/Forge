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
      return (index << 8) | version;
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

      elements[index].data = value;

      return createHandle(index);
   }

   Handle add(T&& value)
   {
      std::size_t index = allocate();

      elements[index].data = std::move(value);

      return createHandle(index);
   }

   template<typename... Args>
   Handle emplace(Args&&... args)
   {
      std::size_t index = allocate();

      elements[index].data.emplace(std::forward<Args>(args)...);

      return createHandle(index);
   }

   bool remove(Handle handle)
   {
      if (Element* element = find(handle))
      {
         element->data.reset();
         freeIndices.push(handle.index);
         return true;
      }

      return false;
   }

   void removeAll()
   {
      for (std::size_t i = 0; i < elements.size(); ++i)
      {
         if (elements[i].data.has_value())
         {
            elements[i].data.reset();
            freeIndices.push(i);
         }
      }
   }

   void clear()
   {
      elements.clear();
      freeIndices = std::queue<std::size_t>();
   }

   T* get(Handle handle)
   {
      if (Element* element = find(handle))
      {
         ASSERT(element->data.has_value());
         return &element->data.value();
      }

      return nullptr;
   }

   const T* get(Handle handle) const
   {
      if (const Element* element = find(handle))
      {
         ASSERT(element->data.has_value());
         return &element->data.value();
      }

      return nullptr;
   }

private:
   struct Element
   {
      std::optional<T> data;
      uint8_t version = 0;
   };

   std::size_t allocate()
   {
      std::size_t index = 0;

      if (freeIndices.empty())
      {
         index = elements.size();
         elements.emplace_back();
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
      ASSERT(index < elements.size());
      ASSERT(index < (1 << 24));
      ASSERT(elements[index].data.has_value());

#if FORGE_DEBUG
      if (elements[index].version == 255)
      {
         LOG_WARNING("Generational array version overflow (index " << index << ")");
      }
#endif // FORGE_DEBUG

      uint8_t version = ++elements[index].version;
      return Handle(static_cast<uint32_t>(index), version);
   }

   Element* find(Handle handle)
   {
      if (handle.index < elements.size())
      {
         Element& element = elements[handle.index];
         if (element.data.has_value() && element.version == handle.version)
         {
            return &element;
         }
      }

      return nullptr;
   }

   const Element* find(Handle handle) const
   {
      if (handle.index < elements.size())
      {
         const Element& element = elements[handle.index];
         if (element.data.has_value() && element.version == handle.version)
         {
            return &element;
         }
      }

      return nullptr;
   }

   std::vector<Element> elements;
   std::queue<std::size_t> freeIndices;
};
