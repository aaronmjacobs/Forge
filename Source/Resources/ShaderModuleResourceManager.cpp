#include "Resources/ShaderModuleResourceManager.h"

#include <PlatformUtils/IOUtils.h>

ShaderModuleResourceManager::ShaderModuleResourceManager(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceManagerBase(graphicsContext, owningResourceManager)
{
}

ShaderModuleHandle ShaderModuleResourceManager::load(const std::filesystem::path& path)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceHelpers::makeCanonical(path))
   {
      std::string canonicalPathString = canonicalPath->string();
      if (std::optional<Handle> cachedHandle = getCachedHandle(canonicalPathString))
      {
         return *cachedHandle;
      }

      std::optional<std::vector<uint8_t>> sourceData = IOUtils::readBinaryFile(*canonicalPath);
      if (sourceData.has_value() && sourceData->size() > 0)
      {
         ShaderModuleHandle handle = emplaceResource(context, *sourceData);
         cacheHandle(canonicalPathString, handle);

#if FORGE_DEBUG
         if (ShaderModule* shaderModule = get(handle))
         {
            shaderModule->setName(ResourceHelpers::getName(*canonicalPath));
         }
#endif // FORGE_DEBUG

         return handle;
      }
   }

   return ShaderModuleHandle();
}
