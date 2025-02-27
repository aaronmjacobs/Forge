#pragma once

class GraphicsContextBase;

#if FORGE_WITH_VULKAN

class VulkanGraphicsContext;

#if FORGE_WITH_STATIC_RHI
using GraphicsContext = VulkanGraphicsContext;
#else
using GraphicsContext = GraphicsContextBase;
#endif

#endif // FORGE_WITH_VULKAN
