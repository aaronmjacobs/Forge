#pragma once

#include <glm/glm.hpp>

#include <span>
#include <utility>
#include <vector>

#define USE_MEMBER_HASH_FUNCTION(type_name)\
template<>\
struct std::hash<type_name>\
{\
   std::size_t operator()(const type_name& value) const\
   {\
      return value.hash();\
   }\
}

namespace Hash
{
   template<typename T>
   inline std::size_t of(const T& value)
   {
      return std::hash<T>{}(value);
   }

   template<typename T>
   inline void combine(std::size_t& hash, const T& value)
   {
      hash ^= Hash::of(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
   }
}

namespace std
{
   template<typename T>
   struct hash<span<T>>
   {
      size_t operator()(span<T> values) const
      {
         size_t hash = values.size();

         for (const auto& value : values)
         {
            Hash::combine(hash, value);
         }

         return hash;
      }
   };

   template<typename T>
   struct hash<vector<T>>
   {
      size_t operator()(const vector<T>& values) const
      {
         span<const T> valuesSpan = values;
         return Hash::of(valuesSpan);
      }
   };

   template<>
   struct hash<glm::vec4>
   {
      size_t operator()(const glm::vec4& value) const
      {
         size_t hash = 0;

         Hash::combine(hash, value.x);
         Hash::combine(hash, value.y);
         Hash::combine(hash, value.z);
         Hash::combine(hash, value.w);

         return hash;
      }
   };
}
