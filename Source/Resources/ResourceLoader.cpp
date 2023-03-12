#include "Resources/ResourceLoader.h"

#include <PlatformUtils/IOUtils.h>

namespace ResourceLoadHelpers
{
   std::optional<std::filesystem::path> makeCanonical(const std::filesystem::path& path)
   {
      std::optional<std::filesystem::path> absolutePath = path.is_absolute() ? path : IOUtils::getAboluteProjectPath(path);
      if (absolutePath)
      {
         std::error_code errorCode;
         std::filesystem::path canonicalPath = std::filesystem::canonical(*absolutePath, errorCode);

         if (!errorCode)
         {
            return canonicalPath;
         }
      }

      return std::nullopt;
   }

#if FORGE_WITH_DEBUG_UTILS
   std::string getName(const std::filesystem::path& path)
   {
      return path.stem().string();
   }
#endif // FORGE_WITH_DEBUG_UTILS
}
