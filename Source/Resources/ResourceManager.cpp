#include "Resources/ResourceManager.h"

#include "Core/Assert.h"

ResourceManager::ResourceManager(const GraphicsContext& graphicsContext)
   : materialLoader(graphicsContext, *this)
   , meshLoader(graphicsContext, *this)
   , shaderModuleLoader(graphicsContext, *this)
   , textureLoader(graphicsContext, *this)
{
   createDefaultTextures();
}

ResourceManager::~ResourceManager()
{
   clearRefCounts(materialRefCounts);
   clearRefCounts(meshRefCounts);
   clearRefCounts(shaderModuleRefCounts);
   clearRefCounts(textureRefCounts);
}

StrongTextureHandle ResourceManager::loadTexture(const std::filesystem::path& path, const TextureLoadOptions& loadOptions, const TextureProperties& properties, const TextureInitialLayout& initialLayout)
{
   TextureHandle handle = textureLoader.load(path, loadOptions, properties, initialLayout);

   if (!handle)
   {
      handle = getDefaultTextureHandle(loadOptions.fallbackDefaultTextureType);
   }

   return StrongTextureHandle(*this, handle);
}

void ResourceManager::createDefaultTextures()
{
   defaultBlackTextureHandle = StrongTextureHandle(*this, textureLoader.createDefault(DefaultTextureType::Black));
   defaultWhiteTextureHandle = StrongTextureHandle(*this, textureLoader.createDefault(DefaultTextureType::White));
   defaultNormalMapTextureHandle = StrongTextureHandle(*this, textureLoader.createDefault(DefaultTextureType::NormalMap));
   defaultAoRoughnessMetalnessMapTextureHandle = StrongTextureHandle(*this, textureLoader.createDefault(DefaultTextureType::AoRoughnessMetalnessMap));
}

TextureHandle ResourceManager::getDefaultTextureHandle(DefaultTextureType type) const
{
   TextureHandle handle;
   switch (type)
   {
   case DefaultTextureType::None:
      break;
   case DefaultTextureType::Black:
      handle = defaultBlackTextureHandle;
      break;
   case DefaultTextureType::White:
      handle = defaultWhiteTextureHandle;
      break;
   case DefaultTextureType::NormalMap:
      handle = defaultNormalMapTextureHandle;
      break;
   case DefaultTextureType::AoRoughnessMetalnessMap:
      handle = defaultAoRoughnessMetalnessMapTextureHandle;
      break;
   default:
      break;
   }

   return handle;
}

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
   auto& refCounts = getRefCounts<T>();

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
