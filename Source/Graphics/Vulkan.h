#pragma once

#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif

#include "Core/Assert.h"

#if PPK_ASSERT_ENABLED && FORGE_DEBUG
#  define VULKAN_HPP_ASSERT ASSERT
#else
#  define VULKAN_HPP_ASSERT(expression) ((void)0)
#endif

#if !defined(VK_ENABLE_BETA_EXTENSIONS)
#  define VK_ENABLE_BETA_EXTENSIONS
#endif

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
