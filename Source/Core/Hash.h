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

#define USE_MEMBER_HASH_FUNCTION_TEMPLATE(template_args, type_name)\
template<template_args>\
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

   template<typename First, typename... Rest>
   inline std::size_t of(const First& first, const Rest&... rest)
   {
      std::size_t hash = of(first);

      (combine(hash, rest), ...);

      return hash;
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
         return Hash::of(value.x, value.y, value.z, value.w);
      }
   };
}
