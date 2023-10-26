#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wnullability-completeness" // Pointer is missing a nullability type specifier (_Nonnull, _Nullable, or _Null_unspecified)
#endif // defined(__clang__)

// Must include directly (can't include Vulkan.h, as it is used in the PCH)
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif // defined(__clang__)
