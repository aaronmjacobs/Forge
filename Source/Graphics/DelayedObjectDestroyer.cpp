#include "Graphics/DelayedObjectDestroyer.h"

#include "Core/Assert.h"
#include "Core/Types.h"

namespace
{
   template<typename T>
   void destroyObject(vk::Device device, T object)
   {
      device.destroy(object);
   }

   void destroyManagedObject(vk::Device device, const DelayedObjectDestroyer::ManagedObject& managedObject)
   {
      ASSERT(managedObject.handle != 0);

#define DESTROY_OBJECT_TYPE_CASE(object_type) case object_type: destroyObject(device, Types::bit_cast<vk::CppType<vk::ObjectType, object_type>::Type>(managedObject.handle)); break
      switch (managedObject.type)
      {
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eSemaphore);
         DESTROY_OBJECT_TYPE_CASE(vk::ObjectType::eFence);
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

   if (handle != 0)
   {
      managedObjectsByFrameIndex[context.getFrameIndex()].emplace_back(handle, type);
   }
}
