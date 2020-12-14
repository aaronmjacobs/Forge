#include "Resources/ShaderModuleResourceManager.h"

#include <PlatformUtils/IOUtils.h>

ShaderModuleHandle ShaderModuleResourceManager::load(const std::filesystem::path& path, const VulkanContext& context)
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
         cacheHandle(handle, canonicalPathString);

         return handle;
      }
   }

   return ShaderModuleHandle();
}
