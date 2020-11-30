#include "Resources/ResourceManagerBase.h"

#include <PlatformUtils/IOUtils.h>

namespace ResourceHelpers
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

      return {};
   }
}
