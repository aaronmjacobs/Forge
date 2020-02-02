#include "Platform/OSUtils.h"

#include <ctime>
#include <vector>

#if FORGE_PLATFORM_MACOS
#include <sys/stat.h>
#endif // FORGE_PLATFORM_MACOS

#if FORGE_PLATFORM_LINUX
#include <cstring>
#include <limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // FORGE_PLATFORM_LINUX

#if FORGE_PLATFORM_WINDOWS
#include <codecvt>
#include <cstring>
#include <locale>
#include <ShlObj.h>
#include <sys/stat.h>
#include <sys/types.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // FORGE_PLATFORM_WINDOWS

namespace
{
   const char* kPathSeparators = "/\\";
}

namespace OSUtils
{
   std::optional<std::string_view> getDirectoryFromPath(std::string_view path)
   {
      size_t pos = path.find_last_of(kPathSeparators);
      if (pos == std::string::npos)
      {
         return {};
      }

#if FORGE_PLATFORM_WINDOWS
      bool isRoot = pos == 2;
#else
      bool isRoot = pos == 0;
#endif

      return isRoot ? path : path.substr(0, pos);
   }

   std::optional<std::string_view> getFileNameFromPath(std::string_view path, bool withExtension)
   {
      size_t slashPos = path.find_last_of(kPathSeparators);
      if (slashPos == std::string::npos)
      {
         return {};
      }

      size_t count = path.size() - slashPos;
      if (!withExtension)
      {
         size_t dotPos = path.find(".", slashPos);
         if (dotPos != std::string::npos)
         {
            count = dotPos - slashPos;
         }
      }

      return path.substr(slashPos + 1, count - 1);
   }

#if FORGE_PLATFORM_LINUX
   std::optional<std::string> getExecutablePath()
   {
      char path[PATH_MAX + 1];

      ssize_t numBytes = readlink("/proc/self/exe", path, PATH_MAX);
      if (numBytes == -1)
      {
         return {};
      }
      path[numBytes] = '\0';

      return std::string(path);
   }

   std::optional<std::string> getAppDataDirectory(std::string_view appName)
   {
      // First, check the HOME environment variable
      char* homePath = secure_getenv("HOME");

      // If it isn't set, grab the directory from the password entry file
      if (!homePath)
      {
         homePath = getpwuid(getuid())->pw_dir;
      }

      if (!homePath)
      {
         return {};
      }

      return std::string(homePath).append("/.config/").append(appName);
   }

   bool setWorkingDirectory(std::string_view dir)
   {
      return chdir(std::string(dir).c_str()) == 0;
   }

   bool createDirectory(std::string_view dir)
   {
      return mkdir(std::string(dir).c_str(), 0755) == 0;
   }
#endif // FORGE_PLATFORM_LINUX

#if FORGE_PLATFORM_WINDOWS
   std::optional<std::string> getExecutablePath()
   {
      TCHAR buffer[MAX_PATH + 1];
      DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
      buffer[length] = '\0';

      std::optional<std::string> executablePath;
      if (length == 0 || length == MAX_PATH || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
         static const DWORD kUnreasonablyLargeStringLength = 32767;
         std::vector<TCHAR> unreasonablyLargeBuffer(kUnreasonablyLargeStringLength + 1);
         length = GetModuleFileName(nullptr, unreasonablyLargeBuffer.data(), kUnreasonablyLargeStringLength);
         unreasonablyLargeBuffer[length] = '\0';

         if (length != 0 && length != kUnreasonablyLargeStringLength && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
         {
            executablePath = std::string(unreasonablyLargeBuffer.data());
         }
      }
      else
      {
         executablePath = std::string(buffer);
      }

      return executablePath;
   }

   std::optional<std::string> getAppDataDirectory(std::string_view appName)
   {
      PWSTR path;
      if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path) != S_OK)
      {
         CoTaskMemFree(path);
         return {};
      }

      std::wstring widePathStr(path);
      CoTaskMemFree(path);

#pragma warning(push)
#pragma warning(disable: 4996) // codecvt is deprecated, but there is no replacement
      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      return converter.to_bytes(widePathStr).append("/").append(appName);
#pragma warning(pop)
   }

   bool setWorkingDirectory(std::string_view dir)
   {
      return SetCurrentDirectory(std::string(dir).c_str()) != 0;
   }

   bool createDirectory(std::string_view dir)
   {
      return CreateDirectory(std::string(dir).c_str(), nullptr) != 0;
   }
#endif // FORGE_PLATFORM_WINDOWS

   bool fixWorkingDirectory()
   {
      if (std::optional<std::string> executablePath = getExecutablePath())
      {
         if (std::optional<std::string_view> executableDir = getDirectoryFromPath(*executablePath))
         {
            return setWorkingDirectory(*executableDir);
         }
      }

      return false;
   }

   bool directoryExists(std::string_view dir)
   {
      struct stat info;
      if (stat(std::string(dir).c_str(), &info) != 0)
      {
         return false;
      }

      return (info.st_mode & S_IFDIR) != 0;
   }

   int64_t getTime()
   {
      static_assert(sizeof(std::time_t) <= sizeof(int64_t), "std::time_t will not fit in an int64_t");

      return static_cast<int64_t>(std::time(nullptr));
   }
}
