#pragma once

#include "Core/Assert.h"

#if PPK_ASSERT_ENABLED
#  define VULKAN_HPP_ASSERT ASSERT
#endif // PPK_ASSERT_ENABLED
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.hpp>
