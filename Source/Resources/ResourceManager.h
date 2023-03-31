#pragma once

#include "ResourceContainer.h"
#include "ResourceTypes.h"

#include "Resources/MaterialLoader.h"
#include "Resources/MeshLoader.h"
#include "Resources/ShaderModuleLoader.h"
#include "Resources/TextureLoader.h"

#include <unordered_map>
#include <unordered_set>

class ResourceManager
{
public:
   ResourceManager(const GraphicsContext& graphicsContext);
   ~ResourceManager();

   // All

   void update()
   {
      materialLoader.updateMaterials();
   }

   // Material

   StrongMaterialHandle loadMaterial(const MaterialParameters& materialParameters)
   {
      return StrongMaterialHandle(*this, materialLoader.load(materialParameters));
   }

   bool unloadMaterial(MaterialHandle handle)
   {
      return materialLoader.unload(handle);
   }

   Material* getMaterial(MaterialHandle handle)
   {
      return materialLoader.get(handle);
   }

   const Material* getMaterial(MaterialHandle handle) const
   {
      return materialLoader.get(handle);
   }

   const MaterialParameters* getMaterialParameters(MaterialHandle handle) const
   {
      return materialLoader.findKey(handle);
   }

   // Mesh

   StrongMeshHandle loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {})
   {
      return StrongMeshHandle(*this, meshLoader.load(path, loadOptions));
   }

   bool unloadMesh(MeshHandle handle)
   {
      return meshLoader.unload(handle);
   }

   Mesh* getMesh(MeshHandle handle)
   {
      return meshLoader.get(handle);
   }

   const Mesh* getMesh(MeshHandle handle) const
   {
      return meshLoader.get(handle);
   }

   const std::string* getMeshPath(MeshHandle handle) const
   {
      const MeshKey* key = meshLoader.findKey(handle);
      return key ? &key->canonicalPath : nullptr;
   }

   // ShaderModule

   StrongShaderModuleHandle loadShaderModule(const std::filesystem::path& path)
   {
      return StrongShaderModuleHandle(*this, shaderModuleLoader.load(path));
   }

   bool unloadShaderModule(ShaderModuleHandle handle)
   {
      return shaderModuleLoader.unload(handle);
   }

   ShaderModule* getShaderModule(ShaderModuleHandle handle)
   {
      return shaderModuleLoader.get(handle);
   }

   const ShaderModule* getShaderModule(ShaderModuleHandle handle) const
   {
      return shaderModuleLoader.get(handle);
   }

   const std::string* getShaderModulePath(ShaderModuleHandle handle) const
   {
      return shaderModuleLoader.findKey(handle);
   }

   // Texture

   StrongTextureHandle loadTexture(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {}, DefaultTextureType fallbackDefaultTextureType = DefaultTextureType::None);

   bool unloadTexture(TextureHandle handle)
   {
      return textureLoader.unload(handle);
   }

   Texture* getTexture(TextureHandle handle)
   {
      return textureLoader.get(handle);
   }

   const Texture* getTexture(TextureHandle handle) const
   {
      return textureLoader.get(handle);
   }

   Texture* getDefaultTexture(DefaultTextureType type)
   {
      return getTexture(getDefaultTextureHandle(type));
   }

   const Texture* getDefaultTexture(DefaultTextureType type) const
   {
      return getTexture(getDefaultTextureHandle(type));
   }

   const std::string* getTexturePath(TextureHandle handle) const
   {
      const TextureKey* key = textureLoader.findKey(handle);
      return key ? &key->canonicalPath : nullptr;
   }

private:
   template<typename T>
   friend class StrongResourceHandle;

   template<typename T>
   using StrongHandleSet = std::unordered_set<StrongResourceHandle<T>*>;

   template<typename T>
   using RefCountMap = std::unordered_map<ResourceHandle<T>, StrongHandleSet<T>>;

   void createDefaultTextures();

   TextureHandle getDefaultTextureHandle(DefaultTextureType type) const;

   template<typename T>
   RefCountMap<T>& getRefCounts();

   template<typename T>
   void clearRefCounts(RefCountMap<T>& refCounts);

   template<typename T>
   void addRef(StrongResourceHandle<T>& strongHandle);

   template<typename T>
   void removeRef(StrongResourceHandle<T>& strongHandle);

   template<typename T>
   bool unload(ResourceHandle<T> handle);

   MaterialLoader materialLoader;
   MeshLoader meshLoader;
   ShaderModuleLoader shaderModuleLoader;
   TextureLoader textureLoader;

   RefCountMap<Material> materialRefCounts;
   RefCountMap<Mesh> meshRefCounts;
   RefCountMap<ShaderModule> shaderModuleRefCounts;
   RefCountMap<Texture> textureRefCounts;

   StrongTextureHandle defaultBlackTextureHandle;
   StrongTextureHandle defaultWhiteTextureHandle;
   StrongTextureHandle defaultNormalMapTextureHandle;
   StrongTextureHandle defaultAoRoughnessMetalnessMapTextureHandle;
};
