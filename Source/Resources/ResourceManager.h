#pragma once

#include "MeshResourceManager.h"
#include "ShaderModuleResourceManager.h"
#include "TextureResourceManager.h"

class ResourceManager
{
public:
   // All
   
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
   
   // Mesh
   
   MeshHandle loadMesh(const std::filesystem::path& path, const GraphicsContext& context)
   {
      return meshResourceManager.load(path, context);
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
   
   // ShaderModule
   
   ShaderModuleHandle loadShaderModule(const std::filesystem::path& path, const GraphicsContext& context)
   {
      return shaderModuleResourceManager.load(path, context);
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
   
   // Texture
   
   TextureHandle loadTexture(const std::filesystem::path& path, const GraphicsContext& context, const TextureProperties& properties = TextureResourceManager::getDefaultProperties(), const TextureInitialLayout& initialLayout = TextureResourceManager::getDefaultInitialLayout())
   {
      return textureResourceManager.load(path, context, properties, initialLayout);
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
   
private:
   MeshResourceManager meshResourceManager;
   ShaderModuleResourceManager shaderModuleResourceManager;
   TextureResourceManager textureResourceManager;
};
