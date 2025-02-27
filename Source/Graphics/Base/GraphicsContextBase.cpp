#include "Graphics/Base/GraphicsContextBase.h"

#include "Graphics/Swapchain.h"

GraphicsContextBase::GraphicsContextBase(Window& window)
{
}

GraphicsContextBase::~GraphicsContextBase()
{
}

void GraphicsContextBase::setSwapchainIndex(uint32_t index)
{
   ASSERT(swapchain && index < swapchain->getImageCount());
   swapchainIndex = index;
}

void GraphicsContextBase::setFrameIndex(uint32_t index)
{
   ASSERT(index < kMaxFramesInFlight);
   frameIndex = index;
}
