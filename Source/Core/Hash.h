#pragma once

#include <span>
#include <utility>
#include <vector>

namespace Hash
{
   template<typename T>
   inline void combine(std::size_t& seed, const T& value)
   {
      std::hash<T> hasher;
      seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
   }
}

namespace std
{
   template<typename T>
   struct hash<span<T>>
   {
      size_t operator()(span<T> values) const
      {
         size_t seed = values.size();

         for (const auto& value : values)
         {
            Hash::combine(seed, value);
         }

         return seed;
      }
   };

   template<typename T>
   struct hash<vector<T>>
   {
      size_t operator()(const vector<T>& values) const
      {
         span<const T> valuesSpan = values;

         hash<span<const T>> hasher;
         return hasher(valuesSpan);
      }
   };
}
