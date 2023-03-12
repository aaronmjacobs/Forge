#include "Resources/ShaderModuleLoader.h"

#include "Graphics/DebugUtils.h"

#include <PlatformUtils/IOUtils.h>

ShaderModuleLoader::ShaderModuleLoader(const GraphicsContext& graphicsContext, ResourceManager& owningResourceManager)
   : ResourceLoader(graphicsContext, owningResourceManager)
{
}

ShaderModuleHandle ShaderModuleLoader::load(const std::filesystem::path& path)
{
   if (std::optional<std::filesystem::path> canonicalPath = ResourceLoadHelpers::makeCanonical(path))
   {
      std::string canonicalPathString = canonicalPath->string();
      if (Handle cachedHandle = container.findHandle(canonicalPathString))
      {
         return cachedHandle;
      }

      std::optional<std::vector<uint8_t>> sourceData = IOUtils::readBinaryFile(*canonicalPath);
      if (sourceData.has_value() && sourceData->size() > 0)
      {
         ShaderModuleHandle handle = container.emplace(canonicalPathString, context, *sourceData);
         NAME_POINTER(context.getDevice(), get(handle), ResourceLoadHelpers::getName(*canonicalPath));

         return handle;
      }
   }

   return ShaderModuleHandle{};
}
