#include "Resources/ResourceManager.h"

#include "Core/Assert.h"

ResourceManager::ResourceManager(const GraphicsContext& graphicsContext)
   : materialLoader(graphicsContext, *this)
   , meshLoader(graphicsContext, *this)
   , shaderModuleLoader(graphicsContext, *this)
   , textureLoader(graphicsContext, *this)
{
}

ResourceManager::~ResourceManager()
{
   clearRefCounts(materialRefCounts);
   clearRefCounts(meshRefCounts);
   clearRefCounts(shaderModuleRefCounts);
   clearRefCounts(textureRefCounts);
}

#define FOR_EACH_RESOURCE_TYPE(resource_type) \
template<> resource_type* ResourceManager::get<resource_type>(ResourceHandle<resource_type> handle) { return get##resource_type(handle); } \
template<> const resource_type* ResourceManager::get<resource_type>(ResourceHandle<resource_type> handle) const { return get##resource_type(handle); }

#include "Resources/ForEachResourceType.inl"

#undef FOR_EACH_RESOURCE_TYPE

template<>
ResourceManager::RefCountMap<Material>& ResourceManager::getRefCounts()
{
   return materialRefCounts;
}

template<>
ResourceManager::RefCountMap<Mesh>& ResourceManager::getRefCounts()
{
   return meshRefCounts;
}

template<>
ResourceManager::RefCountMap<ShaderModule>& ResourceManager::getRefCounts()
{
   return shaderModuleRefCounts;
}

template<>
ResourceManager::RefCountMap<Texture>& ResourceManager::getRefCounts()
{
   return textureRefCounts;
}

template<typename T>
void ResourceManager::clearRefCounts(RefCountMap<T>& refCounts)
{
   for (auto& [handle, set] : refCounts)
   {
      for (StrongResourceHandle<T>* strongHandle : set)
      {
         ASSERT(strongHandle);
         strongHandle->resourceManager = nullptr;
      }
   }

   refCounts.clear();
}

template<>
bool ResourceManager::unload(ResourceHandle<Material> handle)
{
   return unloadMaterial(handle);
}

template<>
bool ResourceManager::unload(ResourceHandle<Mesh> handle)
{
   return unloadMesh(handle);
}

template<>
bool ResourceManager::unload(ResourceHandle<ShaderModule> handle)
{
   return unloadShaderModule(handle);
}

template<>
bool ResourceManager::unload(ResourceHandle<Texture> handle)
{
   return unloadTexture(handle);
}

template<typename T>
void ResourceManager::addRef(StrongResourceHandle<T>& strongHandle)
{
   RefCountMap<T>& refCounts = getRefCounts<T>();

   auto location = refCounts.find(strongHandle.getHandle());
   if (location == refCounts.end())
   {
      location = refCounts.emplace(strongHandle.getHandle(), StrongHandleSet<T>{}).first;
   }
   StrongHandleSet<T>& set = location->second;

   set.emplace(&strongHandle);
}

template<typename T>
void ResourceManager::removeRef(StrongResourceHandle<T>& strongHandle)
{
   auto& refCounts = getRefCounts<T>();

   auto location = refCounts.find(strongHandle.getHandle());
   ASSERT(location != refCounts.end());
   StrongHandleSet<T>& set = location->second;

   ASSERT(set.contains(&strongHandle));
   set.erase(&strongHandle);

   if (set.empty())
   {
      unload(strongHandle.getHandle());
      refCounts.erase(strongHandle.getHandle());
   }
}

#define FOR_EACH_RESOURCE_TYPE(resource_type) \
template void ResourceManager::addRef<resource_type>(StrongResourceHandle<resource_type>& handle); \
template void ResourceManager::removeRef<resource_type>(StrongResourceHandle<resource_type>& handle);

#include "Resources/ForEachResourceType.inl"

#undef FOR_EACH_RESOURCE_TYPE
