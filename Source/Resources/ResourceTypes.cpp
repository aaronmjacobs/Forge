#include "Resources/ResourceTypes.h"

#include "Resources/ResourceManager.h"

template<typename T>
T* StrongResourceHandle<T>::getResource()
{
   if (resourceManager && handle)
   {
      return resourceManager->get<T>(handle);
   }

   return nullptr;
}

template<typename T>
const T* StrongResourceHandle<T>::getResource() const
{
   if (resourceManager && handle)
   {
      return resourceManager->get<T>(handle);
   }

   return nullptr;
}

template<typename T>
void StrongResourceHandle<T>::addRef()
{
   if (resourceManager && handle)
   {
      resourceManager->addRef(*this);
   }
}

template<typename T>
void StrongResourceHandle<T>::removeRef()
{
   if (resourceManager && handle)
   {
      resourceManager->removeRef(*this);
   }

   resourceManager = nullptr;
   handle.reset();
}

#define FOR_EACH_RESOURCE_TYPE(resource_type) \
template resource_type* StrongResourceHandle<resource_type>::getResource(); \
template const resource_type* StrongResourceHandle<resource_type>::getResource() const; \
template void StrongResourceHandle<resource_type>::addRef(); \
template void StrongResourceHandle<resource_type>::removeRef();

#include "Resources/ForEachResourceType.inl"

#undef FOR_EACH_RESOURCE_TYPE
