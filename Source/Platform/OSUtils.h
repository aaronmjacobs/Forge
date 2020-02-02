#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace OSUtils
{
   std::optional<std::filesystem::path> getExecutablePath();
   std::optional<std::filesystem::path> getAppDataDirectory(std::string_view appName);

   bool fixWorkingDirectory();
}
