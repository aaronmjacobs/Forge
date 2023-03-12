#pragma once

#include "ResourceContainer.h"
#include "ResourceTypes.h"

#include "Resources/MaterialLoader.h"
#include "Resources/MeshLoader.h"
#include "Resources/ShaderModuleLoader.h"
#include "Resources/TextureLoader.h"

class ResourceManager
{
public:
   ResourceManager(const GraphicsContext& graphicsContext)
      : materialLoader(graphicsContext, *this)
      , meshLoader(graphicsContext, *this)
      , shaderModuleLoader(graphicsContext, *this)
      , textureLoader(graphicsContext, *this)
   {
   }

   // All

   void update()
   {
      materialLoader.updateMaterials();
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
      return materialLoader.load(materialParameters);
   }

   bool unloadMaterial(MaterialHandle handle)
   {
      return materialLoader.unload(handle);
   }

   void unloadAllMaterials()
   {
      materialLoader.unloadAll();
   }

   void clearMaterials()
   {
      materialLoader.clear();
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
      return materialLoader.findIdentifier(handle);
   }

   // Mesh

   MeshHandle loadMesh(const std::filesystem::path& path, const MeshLoadOptions& loadOptions = {})
   {
      return meshLoader.load(path, loadOptions);
   }

   bool unloadMesh(MeshHandle handle)
   {
      return meshLoader.unload(handle);
   }

   void unloadAllMeshes()
   {
      meshLoader.unloadAll();
   }

   void clearMeshes()
   {
      meshLoader.clear();
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
      return meshLoader.findIdentifier(handle);
   }

   // ShaderModule

   ShaderModuleHandle loadShaderModule(const std::filesystem::path& path)
   {
      return shaderModuleLoader.load(path);
   }

   bool unloadShaderModule(ShaderModuleHandle handle)
   {
      return shaderModuleLoader.unload(handle);
   }

   void unloadAllShaderModules()
   {
      shaderModuleLoader.unloadAll();
   }

   void clearShaderModules()
   {
      shaderModuleLoader.clear();
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
      return shaderModuleLoader.findIdentifier(handle);
   }

   // Texture

   TextureHandle loadTexture(const std::filesystem::path& path, const TextureLoadOptions& loadOptions = {}, const TextureProperties& properties = TextureLoader::getDefaultProperties(), const TextureInitialLayout& initialLayout = TextureLoader::getDefaultInitialLayout())
   {
      return textureLoader.load(path, loadOptions, properties, initialLayout);
   }

   bool unloadTexture(TextureHandle handle)
   {
      return textureLoader.unload(handle);
   }

   void unloadAllTextures()
   {
      textureLoader.unloadAll();
   }

   void clearTextures()
   {
      textureLoader.clear();
   }

   Texture* getTexture(TextureHandle handle)
   {
      return textureLoader.get(handle);
   }

   const Texture* getTexture(TextureHandle handle) const
   {
      return textureLoader.get(handle);
   }

   const std::string* getTexturePath(TextureHandle handle) const
   {
      return textureLoader.findIdentifier(handle);
   }

   std::unique_ptr<Texture> createDefaultTexture(DefaultTextureType type) const
   {
      return textureLoader.createDefault(type);
   }

private:
   MaterialLoader materialLoader;
   MeshLoader meshLoader;
   ShaderModuleLoader shaderModuleLoader;
   TextureLoader textureLoader;
};
