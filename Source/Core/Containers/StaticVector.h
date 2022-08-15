#pragma once

#include "Core/Assert.h"

#include <array>
#include <initializer_list>
#include <span>

template<typename T, std::size_t Capacity>
class StaticVector
{
public:
   StaticVector() = default;

   StaticVector(std::initializer_list<T> list)
   {
      for (const T* value = list.begin(); value < list.end(); ++value)
      {
         push(*value);
      }
   }

   void push(const T& value)
   {
      ASSERT(used < Capacity);
      values[used++] = value;
   }

   void pop()
   {
      ASSERT(used > 0);
      --used;
   }

   T& operator[](std::size_t index)
   {
      ASSERT(index < used);
      return values[index];
   }

   const T& operator[](std::size_t index) const
   {
      ASSERT(index < used);
      return values[index];
   }

   T* data()
   {
      return values.data();
   }

   const T* data() const
   {
      return values.data();
   }

   std::size_t size() const
   {
      return used;
   }

   constexpr std::size_t capacity() const
   {
      return Capacity;
   }

   bool operator==(const StaticVector& other) const
   {
      if (used != other.used)
      {
         return false;
      }

      for (std::size_t i = 0; i < used; ++i)
      {
         if (values[i] != other.values[i])
         {
            return false;
         }
      }

      return true;
   }

   operator std::span<T>()
   {
      return std::span<T>(data(), size());
   }

   operator std::span<const T>() const
   {
      return std::span<const T>(data(), size());
   }

private:
   std::array<T, Capacity> values;
   std::size_t used = 0;
};
