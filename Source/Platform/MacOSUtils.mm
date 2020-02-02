#if !FORGE_PLATFORM_MACOS
#  error "Trying to compile macOS-only source, but 'FORGE_PLATFORM_MACOS' isn't set!"
#endif // !FORGE_PLATFORM_MACOS

#include "Platform/OSUtils.h"

#import <Foundation/Foundation.h>

#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>

namespace OSUtils
{
   std::optional<std::filesystem::path> getExecutablePath()
   {
      std::optional<std::filesystem::path> executablePath;

      uint32_t size = MAXPATHLEN;
      char rawPath[size];
      if (_NSGetExecutablePath(rawPath, &size) == 0)
      {
         char realPath[size];
         if (realpath(rawPath, realPath))
         {
            executablePath = std::filesystem::path(realPath);
         }
      }

      return executablePath;
   }

   std::optional<std::filesystem::path> getAppDataDirectory(std::string_view appName)
   {
      std::optional<std::filesystem::path> appDataDirectory;

      if (NSURL* appSupportURL = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
                                                                 inDomain:NSUserDomainMask
                                                                 appropriateForURL:nil
                                                                 create:YES
                                                                 error:nil])
      {
         appDataDirectory = std::filesystem::path([[appSupportURL path] cStringUsingEncoding:NSASCIIStringEncoding]) / appName;
      }

      return appDataDirectory;
   }
}
