#include "Resources/ResourceTypes.h"

#include "Resources/ResourceManager.h"

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
template void StrongResourceHandle<resource_type>::addRef(); \
template void StrongResourceHandle<resource_type>::removeRef();

#include "Resources/ForEachResourceType.inl"

#undef FOR_EACH_RESOURCE_TYPE
