#include "Graphics/DelayedObjectDestroyer.h"

#include "Core/Assert.h"
#include "Core/Types.h"

namespace
{
   template<typename T>
   void destroyObject(vk::Device device, T object)
   {
      ASSERT(object);
      device.destroy(object);
   }

   template<typename T>
   void freeObject(vk::Device device, T object)
   {
      ASSERT(object);
      device.free(object);
   }

   template<typename T, typename U>
   void freeObject(vk::Device device, U pool, T object)
   {
      ASSERT(pool && object);
      device.free(pool, object);
   }

   void destroyManagedObject(vk::Device device, const DelayedObjectDestroyer::ManagedObject& managedObject)
   {
#define DESTROY_OBJECT_TYPE_CASE(object_type) case object_type: destroyObject(device, Types::bit_cast<vk::CppType<vk::ObjectType, object_type>::Type>(managedObject.handle)); break
#define FREE_OBJECT_TYPE_CASE(object_type) case object_type: freeObject(device, Types::bit_cast<vk::CppType<vk::ObjectType, object_type>::Type>(managedObject.handle)); break
#define FREE_POOLED_OBJECT_TYPE_CASE(pool_type, object_type) case object_type: freeObject(device, Types::bit_cast<vk::CppType<vk::ObjectType, pool_type>::Type>(managedObject.pool), Types::bit_cast<vk::CppType<vk::ObjectType, object_type>::Type>(managedObject.handle)); break
      switch (managedObject.type)
      {
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eSemaphore);
         FREE_POOLED_OBJECT_TYPE_CASE(vk::ObjectType::eCommandPool, vk::ObjectType::eCommandBuffer);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eFence);
         FREE_OBJECT_TYPE_CASE(vk::ObjectType::eDeviceMemory);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eBuffer);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eImage);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eEvent);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eQueryPool);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eBufferView);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eImageView);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eShaderModule);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::ePipelineCache);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::ePipelineLayout);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eRenderPass);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::ePipeline);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eDescriptorSetLayout);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eSampler);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eDescriptorPool);
         FREE_POOLED_OBJECT_TYPE_CASE(vk::ObjectType::eDescriptorPool, vk::ObjectType::eDescriptorSet);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eFramebuffer);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eCommandPool);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eSwapchainKHR);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eSamplerYcbcrConversion);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eDescriptorUpdateTemplate);
      default:
         ASSERT(false, "Invalid object type: %s", vk::to_string(managedObject.type));
         break;
      }
#undef DESTROY_OBJECT_TYPE_CASE
#undef FREE_OBJECT_TYPE_CASE
#undef FREE_POOLED_OBJECT_TYPE_CASE
   }

   void destroyManagedObjects(vk::Device device, std::vector<DelayedObjectDestroyer::ManagedObject>& managedObjects)
   {
      for (DelayedObjectDestroyer::ManagedObject& managedObject : managedObjects)
      {
         destroyManagedObject(device, managedObject);
      }

      managedObjects.clear();
   }
}

DelayedObjectDestroyer::DelayedObjectDestroyer(const GraphicsContext& graphicsContext)
   : context(graphicsContext)
{
}

DelayedObjectDestroyer::~DelayedObjectDestroyer()
{
   for (std::vector<ManagedObject>& managedObjects : managedObjectsByFrameIndex)
   {
      destroyManagedObjects(context.getDevice(), managedObjects);
   }
}

void DelayedObjectDestroyer::onFrameIndexUpdate()
{
   ASSERT(context.getFrameIndex() < managedObjectsByFrameIndex.size());

   destroyManagedObjects(context.getDevice(), managedObjectsByFrameIndex[context.getFrameIndex()]);
}

void DelayedObjectDestroyer::delayedDestroy(uint64_t handle, vk::ObjectType type)
{
   ASSERT(context.getFrameIndex() < managedObjectsByFrameIndex.size());
   ASSERT(type == vk::ObjectType::eSemaphore
      || type == vk::ObjectType::eFence
      || type == vk::ObjectType::eBuffer
      || type == vk::ObjectType::eImage
      || type == vk::ObjectType::eEvent
      || type == vk::ObjectType::eQueryPool
      || type == vk::ObjectType::eBufferView
      || type == vk::ObjectType::eImageView
      || type == vk::ObjectType::eShaderModule
      || type == vk::ObjectType::ePipelineCache
      || type == vk::ObjectType::ePipelineLayout
      || type == vk::ObjectType::eRenderPass
      || type == vk::ObjectType::ePipeline
      || type == vk::ObjectType::eDescriptorSetLayout
      || type == vk::ObjectType::eSampler
      || type == vk::ObjectType::eDescriptorPool
      || type == vk::ObjectType::eFramebuffer
      || type == vk::ObjectType::eCommandPool
      || type == vk::ObjectType::eSwapchainKHR
      || type == vk::ObjectType::eSamplerYcbcrConversion
      || type == vk::ObjectType::eDescriptorUpdateTemplate
      , "Invalid object type for delayed destroy: %s", vk::to_string(type).c_str());

   if (handle != 0)
   {
      managedObjectsByFrameIndex[context.getFrameIndex()].emplace_back(0, handle, type);
   }
}

void DelayedObjectDestroyer::delayedFree(uint64_t pool, uint64_t handle, vk::ObjectType type)
{
   ASSERT(context.getFrameIndex() < managedObjectsByFrameIndex.size());
   ASSERT(type == vk::ObjectType::eCommandBuffer
      || type == vk::ObjectType::eDeviceMemory
      || type == vk::ObjectType::eDescriptorSet
      , "Invalid object type for delayed free: %s", vk::to_string(type).c_str());

   if (handle != 0)
   {
      ASSERT(pool != 0 || type == vk::ObjectType::eDeviceMemory);
      managedObjectsByFrameIndex[context.getFrameIndex()].emplace_back(pool, handle, type);
   }
}
