#include "Resources/ShaderModuleResourceManager.h"

#include "Graphics/DebugUtils.h"

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

         NAME_POINTER(context.getDevice(), get(handle), ResourceHelpers::getName(*canonicalPath));

         return handle;
      }
   }

   return ShaderModuleHandle();
}
