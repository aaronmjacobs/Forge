#pragma once

#include <cstdint>

class Swapchain;
class Window;

class GraphicsContextBase
{
public:
   static const uint32_t kMaxFramesInFlight = 3;

   GraphicsContextBase(Window& window);

   GraphicsContextBase(const GraphicsContextBase& other) = delete;
   GraphicsContextBase(GraphicsContextBase&& other) = delete;

   virtual ~GraphicsContextBase();

   GraphicsContextBase& operator=(const GraphicsContextBase&& other) = delete;
   GraphicsContextBase& operator=(GraphicsContextBase&& other) = delete;

   const Swapchain& getSwapchain() const
   {
      ASSERT(swapchain);
      return *swapchain;
   }

   void setSwapchain(const Swapchain* newSwapchain)
   {
      swapchain = newSwapchain;
   }

   uint32_t getSwapchainIndex() const
   {
      return swapchainIndex;
   }

   void setSwapchainIndex(uint32_t index);

   uint32_t getFrameIndex() const
   {
      return frameIndex;
   }

   virtual void setFrameIndex(uint32_t index);

private:
   const Swapchain* swapchain = nullptr;
   uint32_t swapchainIndex = 0;

   uint32_t frameIndex = 0;
};
