#pragma once

#include "Graphics/GraphicsContext.h"

#include <array>
#include <cstdint>
#include <vector>

class DelayedObjectDestroyer
{
public:
   DelayedObjectDestroyer(const GraphicsContext& graphicsContext);
   ~DelayedObjectDestroyer();

   void onFrameIndexUpdate();

   void delayedDestroy(uint64_t handle, vk::ObjectType type, VmaAllocation allocation = nullptr);
   void delayedFree(uint64_t pool, uint64_t handle, vk::ObjectType type);

   struct ManagedObject
   {
      ManagedObject() = default;

      ManagedObject(uint64_t pool_, uint64_t handle_, vk::ObjectType type_, VmaAllocation allocation_ = nullptr)
         : pool(pool_)
         , handle(handle_)
         , type(type_)
         , allocation(allocation_)
      {
      }

      uint64_t pool = 0;
      uint64_t handle = 0;
      vk::ObjectType type = vk::ObjectType::eUnknown;
      VmaAllocation allocation = nullptr;
   };

private:
   std::array<std::vector<ManagedObject>, GraphicsContext::kMaxFramesInFlight> managedObjectsByFrameIndex;
   const GraphicsContext& context;
};
