#pragma once

#include "Core/Assert.h"
#include "Core/Hash.h"

#include <cstdint>

template<typename T>
class GenerationalArrayHandle
{
public:
   GenerationalArrayHandle() = default;

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
      return (version << 16) | index;
   }

private:
   template<typename U>
   friend class GenerationalArray;

   GenerationalArrayHandle(uint16_t indexValue, uint16_t versionValue)
      : index(indexValue)
      , version(versionValue)
   {
   }

   uint16_t index = 0;
   uint16_t version = 0;
};

USE_MEMBER_HASH_FUNCTION_TEMPLATE(typename T, GenerationalArrayHandle<T>);
