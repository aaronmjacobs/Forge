#pragma once

#include "Core/Assert.h"

#include <array>
#include <cstddef>
#include <memory>
#include <new>

#if FORGE_DEBUG
#  include <cstring>
#endif // FORGE_DEBUG

template<std::size_t Size>
class FrameAllocatorMemory
{
public:
   template<typename T>
   T* allocate(std::size_t elementCount)
   {
      std::size_t sizeBytes = elementCount * sizeof(T);
      ASSERT(sizeBytes > 0);

      void* alignedPointer = data.data() + offset;
      std::size_t space = Size - offset;
      std::align(alignof(T), sizeBytes, alignedPointer, space);
      ASSERT(alignedPointer, "Not enough space for aligned frame allocation (%lu bytes requested, %lu bytes available, %lu byte alignment)", sizeBytes, space, alignof(T));

      if (alignedPointer)
      {
         space -= sizeBytes;
         offset = Size - space;
      }

      return std::launder(reinterpret_cast<T*>(alignedPointer));
   }

#if FORGE_DEBUG
   FrameAllocatorMemory()
   {
      reset();
   }
#endif // FORGE_DEBUG

   void reset()
   {
      offset = 0;

#if FORGE_DEBUG
      std::memset(data.data(), 0xDE, Size);
#endif // FORGE_DEBUG
   }

private:
   std::size_t offset = 0;
   std::array<std::byte, Size> data;
};

class FrameAllocatorBase
{
public:
   static void reset()
   {
      memory.reset();
   }

protected:
   static constexpr std::size_t kNumBytes = 1024 * 1024;

   static thread_local FrameAllocatorMemory<kNumBytes> memory;
};

template<typename T>
class FrameAllocator : public FrameAllocatorBase
{
public:
   using value_type = T;

   T* allocate(std::size_t elementCount) noexcept
   {
      return memory.allocate<T>(elementCount);
   }

   void deallocate(T* first, std::size_t elementCount) noexcept
   {
   }

   void validate_max(std::size_t elementCount, std::size_t containerMaximum) noexcept
   {
      if (elementCount > containerMaximum)
      {
         std::terminate();
      }
   }

   bool operator==(const FrameAllocator<T>& other) const
   {
      return true;
   }
};
