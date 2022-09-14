#pragma once

#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif

#include "Core/Assert.h"

#if PPK_ASSERT_ENABLED
#  define VULKAN_HPP_ASSERT ASSERT
#endif // PPK_ASSERT_ENABLED

#if !defined(VK_ENABLE_BETA_EXTENSIONS)
#  define VK_ENABLE_BETA_EXTENSIONS
#endif

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
