#pragma once

#include "Core/Containers/GenerationalArrayHandle.h"
#include "Core/Hash.h"

#include <memory>

class Material;
class Mesh;
class ResourceManager;
class ShaderModule;
class Texture;

template<typename ResourceType>
struct ResourcePointers
{
   std::unique_ptr<ResourceType> ownedResource;
   ResourceType* referencedResource = nullptr;

   ResourcePointers() = default;

   ResourcePointers(std::unique_ptr<ResourceType> resource)
      : ownedResource(std::move(resource))
      , referencedResource(ownedResource.get())
   {
   }

   ResourcePointers(ResourceType* resource)
      : ownedResource(nullptr)
      , referencedResource(resource)
   {
   }
};

template<typename T>
using ResourceHandle = GenerationalArrayHandle<ResourcePointers<T>>;

template<typename T>
class StrongResourceHandle
{
public:
   StrongResourceHandle() = default;

   StrongResourceHandle(const StrongResourceHandle& other)
      : resourceManager(other.resourceManager)
      , handle(other.handle)
   {
      addRef();
   }

   StrongResourceHandle(StrongResourceHandle&& other)
      : resourceManager(other.resourceManager)
      , handle(other.handle)
   {
      addRef();

      other.removeRef();
   }

   ~StrongResourceHandle()
   {
      removeRef();
   }

   StrongResourceHandle& operator=(const StrongResourceHandle& other)
   {
      removeRef();

      resourceManager = other.resourceManager;
      handle = other.handle;

      addRef();

      return *this;
   }

   StrongResourceHandle& operator=(StrongResourceHandle&& other)
   {
      removeRef();

      resourceManager = other.resourceManager;
      handle = other.handle;

      addRef();

      other.removeRef();

      return *this;
   }

   bool isValid() const
   {
      return resourceManager != nullptr && handle.isValid();
   }

   void reset()
   {
      removeRef();
   }

   ResourceHandle<T> getHandle() const
   {
      return handle;
   }

   T* getResource();
   const T* getResource() const;

   ResourceManager* getResourceManager() const
   {
      return resourceManager;
   }

   operator ResourceHandle<T>() const
   {
      return getHandle();
   }

   explicit operator bool() const
   {
      return isValid();
   }

   bool operator==(const StrongResourceHandle& other) const = default;

   std::size_t hash() const
   {
      size_t hash = 0;

      Hash::combine(hash, resourceManager);
      Hash::combine(hash, handle);

      return hash;
   }

private:
   friend class ResourceManager;

   StrongResourceHandle(ResourceManager& manager, ResourceHandle<T> resourceHandle)
      : resourceManager(&manager)
      , handle(resourceHandle)
   {
      addRef();
   }

   void addRef();
   void removeRef();

   ResourceManager* resourceManager = nullptr;
   ResourceHandle<T> handle;
};

USE_MEMBER_HASH_FUNCTION_TEMPLATE(typename T, StrongResourceHandle<T>);

#define FOR_EACH_RESOURCE_TYPE(resource_type) \
using resource_type##Handle = ResourceHandle<resource_type>; \
using Strong##resource_type##Handle = StrongResourceHandle<resource_type>;

#include "Resources/ForEachResourceType.inl"

#undef FOR_EACH_RESOURCE_TYPE
