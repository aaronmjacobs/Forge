#pragma once

#include "MaterialResourceManager.h"
#include "MeshResourceManager.h"
#include "ShaderModuleResourceManager.h"
#include "TextureResourceManager.h"

class ResourceManager
{
public:
   ResourceManager(const GraphicsContext& graphicsContext)
      : materialResourceManager(graphicsContext, *this)
      , meshResourceManager(graphicsContext, *this)
      , shaderModuleResourceManager(graphicsContext, *this)
      , textureResourceManager(graphicsContext, *this)
   {
   }

   // All

   void update()
   {
      materialResourceManager.updateMaterials();
   }

   void unloadAll()
   {
      unloadAllMeshes();
      unloadAllShaderModules();
      unloadAllTextures();
   }

   void clearAll()
   {
      clearMeshes();
      clearShaderModules();
      clearTextures();
   }

   // Material

   MaterialHandle loadMaterial(const MaterialParameters& materialParameters)
   {
      return materialResourceManager.load(materialParameters);
   }

   bool unloadMaterial(MaterialHandle handle)
   {
      return materialResourceManager.unload(handle);
   }

   void unloadAllMaterials()
   {
      materialResourceManager.unloadAll();
   }

   void clearMaterials()
   {
      materialResourceManager.clear();
   }

   Material* getMaterial(MaterialHandle handle)
   {
      return materialResourceManager.get(handle);
   }

   const Material* getMaterial(MaterialHandle handle) const
   {
      return materialResourceManager.get(handle);
   }

   const MaterialParameters* getMaterialParameters(MaterialHandle handle) const
   {
      return materialResourceManager.getIdentifier(handle);
   }

   // Mesh

   MeshHandle loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {})
   {
      return meshResourceManager.load(path, loadOptions);
   }

   bool unloadMesh(MeshHandle handle)
   {
      return meshResourceManager.unload(handle);
   }

   void unloadAllMeshes()
   {
      meshResourceManager.unloadAll();
   }

   void clearMeshes()
   {
      meshResourceManager.clear();
   }

   Mesh* getMesh(MeshHandle handle)
   {
      return meshResourceManager.get(handle);
   }

   const Mesh* getMesh(MeshHandle handle) const
   {
      return meshResourceManager.get(handle);
   }

   const std::string* getMeshPath(MeshHandle handle) const
   {
      return meshResourceManager.getIdentifier(handle);
   }

   // ShaderModule

   ShaderModuleHandle loadShaderModule(const std::filesystem::path& path)
   {
      return shaderModuleResourceManager.load(path);
   }

   bool unloadShaderModule(ShaderModuleHandle handle)
   {
      return shaderModuleResourceManager.unload(handle);
   }

   void unloadAllShaderModules()
   {
      shaderModuleResourceManager.unloadAll();
   }

   void clearShaderModules()
   {
      shaderModuleResourceManager.clear();
   }

   ShaderModule* getShaderModule(ShaderModuleHandle handle)
   {
      return shaderModuleResourceManager.get(handle);
   }

   const ShaderModule* getShaderModule(ShaderModuleHandle handle) const
   {
      return shaderModuleResourceManager.get(handle);
   }

   const std::string* getShaderModulePath(ShaderModuleHandle handle) const
   {
      return shaderModuleResourceManager.getIdentifier(handle);
   }

   // Texture

   TextureHandle loadTexture(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {}, const TextureProperties& properties = TextureResourceManager::getDefaultProperties(), const TextureInitialLayout& initialLayout = TextureResourceManager::getDefaultInitialLayout())
   {
      return textureResourceManager.load(path, loadOptions, properties, initialLayout);
   }

   bool unloadTexture(TextureHandle handle)
   {
      return textureResourceManager.unload(handle);
   }

   void unloadAllTextures()
   {
      textureResourceManager.unloadAll();
   }

   void clearTextures()
   {
      textureResourceManager.clear();
   }

   Texture* getTexture(TextureHandle handle)
   {
      return textureResourceManager.get(handle);
   }

   const Texture* getTexture(TextureHandle handle) const
   {
      return textureResourceManager.get(handle);
   }

   const std::string* getTexturePath(TextureHandle handle) const
   {
      return textureResourceManager.getIdentifier(handle);
   }

   std::unique_ptr<Texture> createDefaultTexture(DefaultTextureType type) const
   {
      return textureResourceManager.createDefault(type);
   }

private:
   MaterialResourceManager materialResourceManager;
   MeshResourceManager meshResourceManager;
   ShaderModuleResourceManager shaderModuleResourceManager;
   TextureResourceManager textureResourceManager;
};
