#pragma once

#include "Core/Assert.h"
#include "Core/Containers/GenerationalArrayHandle.h"
#include "Core/Log.h"

#include <optional>
#include <queue>
#include <utility>
#include <vector>

template<typename T>
class GenerationalArray
{
public:
   using Handle = GenerationalArrayHandle<T>;

   Handle add(T value)
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

   bool replace(Handle handle, T value)
   {
      if (Element* element = find(handle))
      {
         ASSERT(element->data.has_value());
         element->data = std::move(value);
         return true;
      }

      return false;
   }

   template<typename... Args>
   bool replace(Handle handle, Args&&... args)
   {
      if (Element* element = find(handle))
      {
         ASSERT(element->data.has_value());
         element->data.emplace(std::forward<Args>(args)...);
         return true;
      }

      return false;
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
      uint16_t version = 0;
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
      ASSERT(index < (1 << 16));
      ASSERT(elements[index].data.has_value());

#if FORGE_WITH_DEBUG_UTILS
      if (elements[index].version == std::numeric_limits<uint16_t>::max())
      {
         LOG_WARNING("Generational array version overflow (index " << index << ")");
      }
#endif // FORGE_WITH_DEBUG_UTILS

      uint16_t version = ++elements[index].version;
      return Handle(static_cast<uint16_t>(index), version);
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
