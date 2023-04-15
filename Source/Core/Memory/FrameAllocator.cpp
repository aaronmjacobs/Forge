#include "Core/Memory/FrameAllocator.h"

// static
thread_local FrameAllocatorMemory<FrameAllocatorBase::kNumBytes> FrameAllocatorBase::memory;
