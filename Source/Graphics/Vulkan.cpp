#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif

// Must include directly (can't include Vulkan.h, as it is used in the PCH)
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
