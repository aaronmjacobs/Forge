#if !FORGE_PLATFORM_MACOS
#  error "Trying to compile macOS-only source, but 'FORGE_PLATFORM_MACOS' isn't set!"
#endif // !FORGE_PLATFORM_MACOS

#include "Platform/OSUtils.h"

#import <Foundation/Foundation.h>

#include <CoreServices/CoreServices.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace OSUtils
{
   std::optional<std::string> getExecutablePath()
   {
      std::optional<std::string> executablePath;

      uint32_t size = MAXPATHLEN;
      char rawPath[size];
      if (_NSGetExecutablePath(rawPath, &size) == 0)
      {
         char realPath[size];
         if (realpath(rawPath, realPath))
         {
            executablePath = std::string(realPath);
         }
      }

      return executablePath;
   }

   std::optional<std::string> getAppDataDirectory(std::string_view appName)
   {
      std::optional<std::string> appDataDirectory;

      if (NSURL* appSupportURL = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
                                                                 inDomain:NSUserDomainMask
                                                                 appropriateForURL:nil
                                                                 create:YES
                                                                 error:nil])
      {
         appDataDirectory = std::string([[appSupportURL path] cStringUsingEncoding:NSASCIIStringEncoding]).append("/").append(appName);
      }

      return appDataDirectory;
   }

   bool setWorkingDirectory(std::string_view dir)
   {
      return chdir(std::string(dir).c_str()) == 0;
   }

   bool createDirectory(std::string_view dir)
   {
      return mkdir(std::string(dir).c_str(), 0755) == 0;
   }
}
