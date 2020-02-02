#include "Platform/OSUtils.h"

#if FORGE_PLATFORM_WINDOWS
#include <codecvt>
#include <locale>
#include <vector>
#include <ShlObj.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // FORGE_PLATFORM_WINDOWS

#if FORGE_PLATFORM_LINUX
#include <linux/limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#endif // FORGE_PLATFORM_LINUX

namespace OSUtils
{
#if FORGE_PLATFORM_WINDOWS
   std::optional<std::filesystem::path> getExecutablePath()
   {
      TCHAR buffer[MAX_PATH + 1];
      DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
      buffer[length] = '\0';

      std::optional<std::filesystem::path> executablePath;
      if (length == 0 || length == MAX_PATH || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
         static const DWORD kUnreasonablyLargeStringLength = 32767;
         std::vector<TCHAR> unreasonablyLargeBuffer(kUnreasonablyLargeStringLength + 1);
         length = GetModuleFileName(nullptr, unreasonablyLargeBuffer.data(), kUnreasonablyLargeStringLength);
         unreasonablyLargeBuffer[length] = '\0';

         if (length != 0 && length != kUnreasonablyLargeStringLength && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
         {
            executablePath = std::filesystem::path(unreasonablyLargeBuffer.data());
         }
      }
      else
      {
         executablePath = std::filesystem::path(buffer);
      }

      return executablePath;
   }

   std::optional<std::filesystem::path> getAppDataDirectory(std::string_view appName)
   {
      std::optional<std::filesystem::path> appDataDirectory;

      PWSTR path = nullptr;
      if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &path) == S_OK)
      {
#pragma warning(push)
#pragma warning(disable: 4996) // codecvt is deprecated, but there is no replacement
         std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
         appDataDirectory = std::filesystem::path(converter.to_bytes(std::wstring(path))) / appName;
#pragma warning(pop)
      }
      CoTaskMemFree(path);

      return appDataDirectory;
   }
#endif // FORGE_PLATFORM_WINDOWS

#if FORGE_PLATFORM_LINUX
   std::optional<std::filesystem::path> getExecutablePath()
   {
      std::optional<std::filesystem::path> executablePath;

      char path[PATH_MAX + 1];

      ssize_t numBytes = readlink("/proc/self/exe", path, PATH_MAX);
      if (numBytes >= 0 && numBytes <= PATH_MAX)
      {
         path[numBytes] = '\0';
         executablePath = std::filesystem::path(path);
      }

      return executablePath;
   }

   std::optional<std::filesystem::path> getAppDataDirectory(std::string_view appName)
   {
      std::optional<std::filesystem::path> appDataDirectory;

      // First, check the HOME environment variable
      char* homePath = secure_getenv("HOME");

      // If it isn't set, grab the directory from the password entry file
      if (!homePath)
      {
         if (struct passwd* pw = getpwuid(getuid()))
         {
            homePath = pw->pw_dir;
         }
      }

      if (homePath)
      {
         appDataDirectory = std::filesystem::path(homePath) / ".config" / appName;
      }

      return appDataDirectory;
   }
#endif // FORGE_PLATFORM_LINUX

   bool fixWorkingDirectory()
   {
      if (std::optional<std::filesystem::path> executablePath = getExecutablePath())
      {
         std::error_code errorCode;
         std::filesystem::current_path(executablePath->parent_path(), errorCode);

         return !errorCode;
      }

      return false;
   }
}
