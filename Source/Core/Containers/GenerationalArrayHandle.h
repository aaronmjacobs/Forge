#pragma once

#include "Core/Assert.h"
#include "Core/Hash.h"

#include <cstdint>

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

   bool operator==(const GenerationalArrayHandle& other) const = default;

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

USE_MEMBER_HASH_FUNCTION_TEMPLATE(typename T, GenerationalArrayHandle<T>);
