#pragma once

#include "Core/Hash.h"

#include <cstdint>

class DelegateHandle
{
public:
   static DelegateHandle create();

   bool isValid() const
   {
      return id != 0;
   }

   void invalidate()
   {
      id = 0;
   }

   friend bool operator==(const DelegateHandle& first, const DelegateHandle& second) = default;

   std::size_t hash() const
   {
      return Hash::of(id);
   }

private:
   static uint64_t counter;

   uint64_t id = 0;
};

USE_MEMBER_HASH_FUNCTION(DelegateHandle);
