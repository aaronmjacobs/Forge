#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace OSUtils
{
   std::optional<std::string_view> getDirectoryFromPath(std::string_view path);
   std::optional<std::string_view> getFileNameFromPath(std::string_view path, bool withExtension);

   std::optional<std::string> getExecutablePath();
   std::optional<std::string> getAppDataDirectory(std::string_view appName);

   bool setWorkingDirectory(std::string_view dir);
   bool fixWorkingDirectory();

   bool directoryExists(std::string_view dir);
   bool createDirectory(std::string_view dir);

   int64_t getTime();
}
